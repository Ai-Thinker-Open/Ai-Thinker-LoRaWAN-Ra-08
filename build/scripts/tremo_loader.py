import argparse
import serial
import os
import sys
import time
import struct
import zlib
import binascii


class TremoLoader(object):
    DEFAULT_PORT = "COM4"
    DEFAULT_BAUD = 921600

    CMD_SYNC = 1
    CMD_JUMP = 2
    CMD_FLASH = 3
    CMD_ERASE = 4
    CMD_VERIFY = 5
    CMD_WROTP = 6
    CMD_RDOTP = 7
    CMD_WROPT0 = 8
    CMD_RDOPT0 = 9
    CMD_WROPT1 = 10
    CMD_RDOPT1 = 11
    CMD_REBOOT = 12
    CMD_SN = 13
    CMD_WRREG = 14
    CMD_RDREG = 15
    CMD_BAUDRATE = 16
    CMD_VERSION = 17

    def __init__(self, port=DEFAULT_PORT, baud=DEFAULT_BAUD):
        self.ser = serial.Serial(port, baud, timeout=5)

    def wait_response(self):
        header = self.ser.read(4)
        if header == b'':
            raise CmdException("Read response header timeout")
        (start, status, rsp_data_len) = struct.unpack('<BBH', header)
        if start != 0xFE:
            raise CmdException("Wrong response packet")
        if rsp_data_len > 512:
            raise CmdException("Wrong response packet length")

        remain_data = self.ser.read(rsp_data_len+5)
        if remain_data == b'':
            raise CmdException("Read response data timeout")

        pkt = header + remain_data
        #print('recv Packed Value :', binascii.hexlify(pkt))
        (crc, end) = struct.unpack('<IB', pkt[4+rsp_data_len:])

        if end != 0xEF:
            raise CmdException("Wrong response packet")

        checksum = zlib.crc32(pkt[:4+rsp_data_len]) & 0xFFFFFFFF
        if crc != checksum:
            raise CmdException("Response crc error")

        return status, pkt[4:4+rsp_data_len]

    def requeset(self, cmd, data=b""):
        pkt = struct.pack(b'<BBH', 0xFE, cmd, len(data)) + data
        checksum = zlib.crc32(pkt) & 0xFFFFFFFF
        pkt += struct.pack(b'<IB', checksum, 0xEF)
        #print('send Packed Value :', binascii.hexlify(pkt))
        self.ser.flushInput()
        self.ser.write(pkt)

    def sync(self):
        self.requeset(self.CMD_SYNC)
        self.wait_response()

    def hw_reset(self, mode=0):
        if mode:
            self.ser.setDTR(True)  # gpio2 1
            self.ser.setRTS(True)  # rst 0
            time.sleep(0.1)
            self.ser.setRTS(False)  # rst 1
            time.sleep(0.1)
            self.ser.setDTR(False)  # gpio2 0
        else:
            self.ser.setRTS(True)
            time.sleep(0.1)
            self.ser.setRTS(False)

    def connect(self, retry=3):
        print('Connecting...')
        sys.stdout.flush()

        for _ in range(retry):
            try:
                self.hw_reset(1)

                last_error = None
                self.ser.flushInput()
                self.ser.flushOutput()
                self.sync()
                break
            except CmdException as e:
                sys.stdout.flush()
                time.sleep(0.05)
                last_error = e

        if last_error is not None:
            self.ser.close()
            print('Connect failed')
            raise CmdException('Connect failed: %s' % last_error)
        print('Connected')

    def erase(self, addr, size):
        self.requeset(self.CMD_ERASE, struct.pack('<II', addr, size))
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Erase error")

    def flash(self, addr, image_data):
        self.requeset(self.CMD_FLASH, struct.pack('<II', addr, len(image_data)) + image_data)
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Flash error")

    def verify(self, addr, size, checksum):
        self.requeset(self.CMD_VERIFY, struct.pack('<III', addr, size, checksum))
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Verify error")

    def jump(self, addr):
        self.requeset(self.CMD_JUMP, struct.pack('<I', addr))
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Jump error")

    def write_otp(self, addr, data):
        self.requeset(self.CMD_WROTP, struct.pack('<II', addr, len(data)) + data)
        ret, rsp = self.wait_response()
        if ret != 0:
            raise CmdException("Write_otp error")

    def read_otp(self, addr, size):
        self.requeset(self.CMD_RDOTP, struct.pack('<II', addr, size))
        ret, rsp = self.wait_response()
        if ret != 0:
            raise CmdException("Read_otp error")
        return rsp

    def reboot(self, mode=0):
        self.requeset(self.CMD_REBOOT, struct.pack('B', mode))
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Reboot error")

    def read_sn(self):
        self.requeset(self.CMD_SN)
        ret, rsp = self.wait_response()
        if ret != 0:
            raise CmdException("Read_sn error")
        return rsp

    def write_reg(self, addr, value):
        self.requeset(self.CMD_WRREG, struct.pack('<II', addr, value))
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Write_reg error")

    def read_reg(self, addr):
        self.requeset(self.CMD_RDREG, struct.pack('<I', addr))
        ret, rsp = self.wait_response()
        if ret != 0:
            raise CmdException("Read_reg error")
        return struct.unpack('<I', rsp)

    def set_baudrate(self, baud):
        self.requeset(self.CMD_BAUDRATE, struct.pack('<I', baud))
        ret, _ = self.wait_response()
        if ret != 0:
            raise CmdException("Set_baudrate error")

        try:
            self.ser.baudrate = baud
        except IOError:
            raise CmdException("Set baudrate error")

    def read_version(self):
        self.requeset(self.CMD_VERSION)
        ret, rsp = self.wait_response()
        if ret != 0:
            raise CmdException("Read_version error")
        return struct.unpack('<I', rsp)


class CmdException(Exception):
    pass


def arg_int(x):
    return int(x, 0)


def get_crc32(filename):
    with open(filename, 'rb') as f:
        return zlib.crc32(f.read()) & 0xFFFFFFFF


def tremo_erase(args):
    tremo = TremoLoader(args.port)
    tremo.connect()
    tremo.set_baudrate(args.baud)
    tremo.erase(args.address, args.size)


def tremo_flash(args):
    # check args
    download_files = []
    for i in range(0, len(args.addr_file), 2):
        try:
            address = int(args.addr_file[i], 0)
        except Exception:
            raise Exception('Address "%s" must be a number' % args.addr_file[i])

        if os.access(args.addr_file[i+1], os.R_OK) is False:
            raise Exception('failed to read the file: %s' % args.addr_file[i+1])
        download_files.append((address, args.addr_file[i+1]))

    # flash
    tremo = TremoLoader(args.port)
    tremo.connect()
    tremo.set_baudrate(args.baud)
    for address, filename in download_files:
        image_size = os.path.getsize(filename)
        image_checksum = get_crc32(filename)
        tremo.erase(address, image_size)

        flash_addr = address
        with open(filename, 'rb') as f:
            l = 0
            while True:
                line_data = f.read(512)
                if line_data == b'':
                    break

                tremo.flash(flash_addr, line_data)
                flash_addr += len(line_data)
                l += len(line_data)
                print("send: ", l)
        tremo.verify(address, image_size, image_checksum)
    tremo.reboot(0)


def tremo_write_otp(args):
    tremo = TremoLoader(args.port)
    tremo.connect()
    tremo.set_baudrate(args.baud)
    tremo.write_otp(args.address, binascii.unhexlify(args.data))


def tremo_read_otp(args):
    tremo = TremoLoader(args.port)
    tremo.connect()
    tremo.set_baudrate(args.baud)
    return tremo.read_otp(args.address, args.size)

def tremo_read_sn(args):
    tremo = TremoLoader(args.port)
    tremo.connect()
    tremo.set_baudrate(args.baud)
    return tremo.read_sn()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '--port', '-p',
        help='serial port',
        default='COM4')

    parser.add_argument(
        '--baud', '-b',
        help='baud rate',
        type=int,
        default=921600)

    subparsers = parser.add_subparsers(
        dest='command',
        help='run tremo_loader.py {command} -h')

    # erase
    parser_erase = subparsers.add_parser(
        'erase',
        help='erase the flash')
    parser_erase.add_argument('address', help='flash address to erase', type=arg_int)
    parser_erase.add_argument('size', help='size', type=arg_int)

    # flash
    parser_flash = subparsers.add_parser(
        'flash',
        help='erase and write the flash')
    parser_flash.add_argument('addr_file', metavar='<address> <filename>',
                                    help='address and filename, eg. 0x08000000 app.bin', nargs='+')

    # write_otp
    parser_write_otp = subparsers.add_parser(
        'write_otp',
        help='write the flash otp area')
    parser_write_otp.add_argument('address', help='flash otp address', type=arg_int)
    parser_write_otp.add_argument('data', help='the data to be written')

    # read_otp
    parser_read_otp = subparsers.add_parser(
        'read_otp',
        help='read the flash otp area')
    parser_read_otp.add_argument('address', help='flash otp address', type=arg_int)
    parser_read_otp.add_argument('size', help='size', type=arg_int)

    # read_sn
    parser_read_sn = subparsers.add_parser(
        'read_sn',
        help='read the chip serial number')

    args = parser.parse_args()

    try:
        if args.command == 'erase':
            tremo_erase(args)
            print('Erase successfully')
        elif args.command == 'flash':
            tremo_flash(args)
            print('Download files successfully')
        elif args.command == 'write_otp':
            tremo_write_otp(args)
            print('Write OTP successfully')
        elif args.command == 'read_otp':
            data = tremo_read_otp(args)
            print('OTP data: %s' % binascii.hexlify(data))
        elif args.command == 'read_sn':
            sn = tremo_read_sn(args)
            print('The SN is: %s' % binascii.hexlify(sn))
    except Exception as e:
        print(str(e))



