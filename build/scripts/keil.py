import os
import sys
import string
import re, shutil
import configparser
import xml.etree.ElementTree as etree
from xml.etree.ElementTree import SubElement
from xml_format import gen_indent

# Global  Definitions
TEMPLATE_PROJX = "build/scripts/asr_template.uvprojx"
TEMPLATE_OPTX = "build/scripts/asr_template.uvoptx"

config = ""
outputname_string = ""
tremo_sdk_path = ""
target_name = 'project'
# Elements need to be changed:
element_dict = {
    "TargetName": { "xpath": "Targets/Target/TargetName" },
    "Device": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/Device" },
    "Vendor": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/Vendor" },
    "OutputName": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/OutputName" },
    "CreateExecutable": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/CreateExecutable" },
    "CreateLib": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/CreateLib" },
    "RunUserProg1": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/AfterMake/RunUserProg1" },
    "RunUserProg2": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/AfterMake/RunUserProg2" },
    "UserProg1Name": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/AfterMake/UserProg1Name" },
    "UserProg2Name": { "xpath": "Targets/Target/TargetOption/TargetCommonOption/AfterMake/UserProg2Name" },
    "ScatterFile": { "xpath": "Targets/Target/TargetOption/TargetArm/LDarm/ScatterFile" },
    "Misc": { "xpath": "Targets/Target/TargetOption/TargetArm/LDarm/Misc" },
    "GCPUTYP": { "xpath": "Targets/Target/TargetOption/TargetArm/ArmMisc/GCPUTYP" },
    "ADefine": { "xpath": "Targets/Target/TargetOption/TargetArm/Aarm/VariousControls/Define" },
    "Define": { "xpath": "Targets/Target/TargetOption/TargetArm/Carm/VariousControls/Define" },
    "IncludePath": { "xpath": "Targets/Target/TargetOption/TargetArm/Carm/VariousControls/IncludePath" },
    "MiscControls_cflags": { "xpath": "Targets/Target/TargetOption/TargetArm/Carm/VariousControls/MiscControls" },
    "MiscControls_asmflags": { "xpath": "Targets/Target/TargetOption/TargetArm/Aarm/VariousControls/MiscControls" },
}

def create_file(data, filename):
    """ Create *_opts files """
    with open(filename, "w") as f:
        f.write(data)

def get_element_value(element_dict):
    global outputname_string
    """Get elements value """
    # TargetName, OutputName = app@board
    element_dict["TargetName"]["value"] = target_name
    element_dict["OutputName"]["value"] = outputname_string

    element_dict["CreateExecutable"]["value"] = eval(config['settings']['createexe'])
    element_dict["CreateLib"]["value"] = eval(config['settings']['createLib'])

    element_dict["RunUserProg1"]["value"] = eval(config['settings']['runuser1'])
    element_dict["RunUserProg2"]["value"] = eval(config['settings']['runuser2'])

    element_dict["UserProg1Name"]["value"] = eval(config['settings']['runuser1_pro']).replace("/","\\")
    element_dict["UserProg2Name"]["value"] = eval(config['settings']['runuser2_pro']).replace("/","\\")

    # Device = $(NAME)_KEIL_DEVICE that defined in board makefile
    element_dict["Device"]["value"] = 'ARMCM4_FP'
    # Vendor = $(NAME)_KEIL_VENDOR that defined in board makefile
    element_dict["Vendor"]["value"] = 'ARM'

    # ScatterFile = the matched part in LDFLAGS "--scatter=(*.sct)"
    # Misc = global ldflags

    element_dict["ScatterFile"]["value"] = eval(config['settings']['ld_files']).replace("/","\\")

    element_dict["Misc"]["value"] = eval(config['settings']['ld_misc'])

    # GCPUTYP = HOST_ARCH that defined in board makefile
    element_dict["GCPUTYP"]["value"] = eval(config['settings']['host_arch'])
    # include path
    element_dict["IncludePath"]["value"] = eval(config['settings']['include_path']).replace("/","\\")

    # Define = global defines splitted by ","
    element_dict["Define"]["value"] = eval(config['settings']['defines']).replace("-D","")

    element_dict["ADefine"]["value"] = eval(config['settings']['adefines']).replace("-D","")
    # MiscControls_cflags = global cflags
    element_dict["MiscControls_cflags"]["value"] = eval(config['settings']['cMisc'])

    # MiscControls_asmflags = global asmflags
    element_dict["MiscControls_asmflags"]["value"] = eval(config['settings']['aMisc'])

def file_type_value(fn):
    """ Mapping Number and Filetype """
    if fn.endswith('.h'):
        return 5
    if fn.endswith('.s') or fn.endswith('.S'):
        return 2
    if fn.endswith('.lib') or fn.endswith('.a'):
        return 4
    if fn.endswith('.cpp') or fn.endswith('.cxx'):
        return 8
    if fn.endswith('.c') or fn.endswith('.C'):
        return 1
    return 5

def add_group(parent, name, files, project_path):
    """Create Group SubElenent"""
    cur_encoding = sys.getfilesystemencoding()
    group = SubElement(parent, 'Group')
    group_name = SubElement(group, 'GroupName')
    group_name.text = name
    files_label = SubElement(group, 'Files')
    for f in files:
        file = SubElement(files_label, 'File')
        file_name = SubElement(file, 'FileName')
        name = os.path.basename(f)
        file_name.text = name.encode('utf-8').decode(cur_encoding)
        file_type = SubElement(file, 'FileType')
        file_type.text = '%d' % file_type_value(name)
        file_path = SubElement(file, 'FilePath')
        f = f.replace('/','\\')
        file_path.text = f.encode('utf-8').decode(cur_encoding)
    return group

def changeItemForMcu(tree, xpath, value):
    """Set Element's value"""
    element = tree.find(xpath)
    element.text = value

def gen_file_group():
    """Generate group"""
    group_pack = []
    src_files = re.split('[, \']', config['settings']['src'])
    lib_files = re.split('[, \']', config['settings']['lib'])
    group_dict_user = {}
    group_dict_user['name'] = 'user'
    group_dict_user['src'] = []
    group_dict_lib = {}
    group_dict_lib['name'] = 'lib'
    group_dict_lib['src'] = []

    src_files = [item for item in filter(lambda x: x != '', src_files)]
    lib_files = [item for item in filter(lambda x: x != '', lib_files)]

    if len(src_files) != 0:
        for src in src_files:
            src_split = re.split('[./]', src)
            src_split_filter = [item for item in filter(lambda x: x != '', src_split)]
            if src_split_filter[0] == 'projects':
                group_dict_user['src'].append(src)
            else:
                group_find = False
                for group in group_pack:
                    if src_split_filter[0] == group['name']:
                        group_find = True
                        group['src'].append(src)

                if not group_find:
                    group_dict = {}
                    group_dict['name'] = src_split_filter[0]
                    group_dict['src'] = []
                    group_dict['src'].append(src)
                    group_pack.append(group_dict)

        group_pack.append(group_dict_user)
    
    if len(lib_files) != 0:
        for lib in lib_files:
            group_dict_lib['src'].append(lib)
        group_pack.append(group_dict_lib)

    return group_pack

def gen_projxfile(tree, target, project_group):
    project_path = os.path.dirname(os.path.abspath(target))

    root = tree.getroot()
    get_element_value(element_dict)

    for key in element_dict:
        xpath = element_dict[key]["xpath"]
        value = element_dict[key]["value"]
        changeItemForMcu(tree, xpath, value)
    # add group
    groups = tree.find('Targets/Target/Groups')
    if groups is None:
        groups = SubElement(tree.find('Targets/Target'), 'Groups')
    groups.clear() # clean old groups

    lib_group=[]
    src_group=[]
    for group_lite in project_group:
        if 'lib' in group_lite['name']:
            lib_group.append(group_lite)
        else:
            src_group.append(group_lite)

    for single_group in src_group:
        # don't add an empty group
        if len(single_group['src'])!= 0:
            group_tree = add_group(groups, single_group['name'],single_group['src'] , project_path)
    for single_group in lib_group:
        # don't add an empty group
        if len(single_group['src'])!= 0:
            group_tree = add_group(groups, single_group['name'],single_group['src'] , project_path)

    gen_indent(root)

    with open(target, 'wb') as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n'.encode())
        f.write(etree.tostring(root, encoding='utf-8'))

def gen_optxfile(optx_tree, optx_file):
    TargetName = optx_tree.find('Target/TargetName')
    TargetName.text = target_name
    if eval(config['settings']['ini_files']):
        TargetName = optx_tree.find('Target/TargetOption/DebugOpt/tIfile')
        TargetName.text = eval(config['settings']['ini_files']).replace("/","\\")
    with open(optx_file, "wb") as f:
        f.write(etree.tostring(optx_tree.getroot(), encoding='utf-8'))

def main():
    global config,outputname_string,target_name,tremo_sdk_path
    tremo_sdk_path = sys.argv[1]
    config_file = sys.argv[2]
    target_name = sys.argv[3]
    outputname_string = sys.argv[4]
    
    config = configparser.ConfigParser()
    config.read_file(open(config_file))

    projx_file = "./%s.uvprojx" % (target_name)
    optx_file = projx_file.replace('.uvprojx', '.uvoptx')

    projx_tree = etree.parse(tremo_sdk_path + "/" + TEMPLATE_PROJX)
    optx_tree = etree.parse(tremo_sdk_path + "/" + TEMPLATE_OPTX)

    pro_group = gen_file_group()

    # create uvprojx file
    gen_projxfile(projx_tree, projx_file, pro_group)

    # create uvoptx file
    gen_optxfile(optx_tree, optx_file)

    print ("Keil project created at %s" % (projx_file))

if __name__ == "__main__":
    main()
