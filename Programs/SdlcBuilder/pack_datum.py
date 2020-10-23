#!/usr/bin/env python
"""
Example: (convert raw files to dvpk or dvpls or generate meta or both. Read arguments from .properties file)
$>python Programs/SdlcBuilder/pack_datum.py --config Data/resource.properties

Example: (convert local files to output dir *.dvpl without config)
$>python Programs/SdlcBuilder/pack_datum.py --input-files base_pack_descriptors.json gfx_descriptors.json
 map-descriptors.json tank_descriptors.json --output_dvpl_dir data/ios_res/ --output-meta-path /data/ios_res/local.db

Example: (convert local files to output remote.dvpl pack without config )
$>python Programs/SdlcBuilder/pack_datum.py --input-file base_pack_descriptors.json --input-file gfx_descriptors.json
  --input-file map-descriptors.json --input-file tank_descriptors.json --output_pack data/remote.dvpk

Example: Using config file(android_superpack_build_on_windows.properties) file content (input files in one line!):
--input-files=./Scripts/sdlc/descriptors_extended/battle_level_descriptors.json \
./Scripts/sdlc/descriptors_extended/fake_descriptors.json \
./Scripts/sdlc/descriptors_extended/general_pack_descriptors.json \
./Scripts/sdlc/descriptors_extended/gfx_descriptors.json \
./Scripts/sdlc/descriptors_extended/ingame_voice_descriptors.json \
./Scripts/sdlc/descriptors_extended/map_descriptors.json \
./Scripts/sdlc/descriptors_extended/tank_descriptors.json
--excluded=./Scripts/sdlc/descriptors_extended/exclude/exclude.json
--input-resource-archiver=Tools/win7/ResourceArchiver
--input-data-dir=./DataExtended
--output-dvpk=./remote_extended.dvpk
--output-meta=./remote_extended.db
EOF
#
# Now you can build remote_extended.dvpk using command line (example):
# $>C:\Python27\python.exe D:/j/dava.framework/Programs/SdlcBuilder/pack_datum.py --config \
d:\j\dava.framework\Programs\SdlcBuilder\test_superpack.properties
"""


import argparse
import sqlite3
import sys
import os
import subprocess

#pip
try:
    import yaml
except ImportError:
    print("error: PyYAML probably is not installed. Attempting to install it via pip.")
    if subprocess.call(["python", "-m", "pip", "install", "PyYAML"]) != 0:
        print("yaml installation failed. Check your imports, python & pip installation and network connection.")
        exit(1)

import yaml


def create_parser():
    parser = argparse.ArgumentParser(fromfile_prefix_chars='@',
                                     description='Process input description files to produce dvpl or dvpk files.')
    parser.add_argument('--config', metavar='file.properties', type=argparse.FileType('r'))
    parser.add_argument('--input-resource-archiver', metavar='resource_archiver.exe', dest='resource_archiver',
                        help='full or relative path to ResourceArchiver binary')
    parser.add_argument('--input-files', type=str, nargs='*', metavar='file.json', dest='input_files',
                        help='list of input description files')
    parser.add_argument('--input-excluded', type=str, nargs='?', metavar='excluded.json', dest='input_excluded',
                        help='json file with list of excluded files')
    parser.add_argument('--input-data-dir', type=str, metavar='data_dir',
                        help='path to input data source directory')
    parser.add_argument('--dummyFileData', action='store_true')  # if flag is set args.dummyFileData == True
    output_group = parser.add_mutually_exclusive_group()
    output_group.add_argument('--output-dvpl-dir', type=str, metavar='*.dvpl_dir', nargs='?',
                              help='path to output *.dvpl directory')
    output_group.add_argument('--output-dvpk', type=str, metavar='file.dvpk', nargs='?',
                              help='path to output *.dvpk file')
    output_group.add_argument('--output-files_dir', type=str, metavar='*.*_dir', nargs='?',
                              help='path to output *.* files')
    parser.add_argument('--output-meta', type=str, metavar='local.db', nargs='?', help='path to output *.db file')
    parser.add_argument('--tmp_dir', type=str, metavar='tmp_dir', default='.tmp_pack', help='path to tmp directory'
                                                                                            ' for intermediate files '
                                                                                            'like remote.db.meta')
    return parser


def parse_args():
    parser = create_parser()
    args = parser.parse_args()

    if args.config:
        from_file_params_key = '@' + args.config.name
        parser = create_parser()
        full_args = parser.parse_args([from_file_params_key])
    else:
        parser = create_parser()
        full_args = parser.parse_args()
    return full_args


def generate_meta(input_data_dir, input_files, input_excluded, output_meta):
    # load excluded files list from json file
    excluded_files = load_excluded_files_list(input_excluded)
    # read pack descriptors from json input files
    # filter packs with excluded list
    # convert dependencies from list of strings to set of pack objects
    all_packs = load_all_descriptors(input_files, excluded_files, input_data_dir)
    # generate list of all files from all packs
    # for every file collect set of packs
    all_files_dict = build_all_files_dict(all_packs)
    # for every file with more then 1 pack
    # find all such files(with same packs set)
    # extract it to new shared generated pack
    # add new generated pack to dependencies set of extracted files
    build_shared_packs(all_files_dict, all_packs)
    # generate (sqlite db) for every pack and every file
    # sort file names for every pack
    # pack sort by name too.
    build_database(input_data_dir, all_packs, output_meta)


def generate_dvpk(resource_archiver, input_data_dir, input_files, input_excluded, output_dvpk, output_meta,
                  dummy_file_data):
    # generate meta.db
    generate_meta(input_data_dir, input_files, input_excluded, output_meta)
    # generate remote.dvpk file from meta.db and directory with files
    call_archiver(resource_archiver, input_data_dir, output_meta, dummy_file_data, output_dvpk)


class FileInfo(object):
    def __init__(self, file_path, pack):
        self.path = file_path
        self.packs = {pack}


def build_all_files_dict(all_packs):
    all_file_dict = dict()  # dict

    for pack in all_packs:
        for file_path in pack.files:
            if file_path in all_file_dict:
                file_info = all_file_dict[file_path]
                if pack not in file_info.packs:
                    file_info.packs.add(pack)  # this file_path found in more than one pack
            else:
                file_info = FileInfo(file_path, pack)
                all_file_dict[file_path] = file_info

    # check every pack dependencies successfully converted from list to set type
    assert(all(type(pack.dependencies) is set for pack in all_packs))
    return all_file_dict


def build_shared_packs(all_files_dict, all_packs):
    auto_pack_index = 0

    for file_path, file_info in all_files_dict.iteritems():
        # if same file exist in more then one pack we extract it to new dependent pack
        if len(file_info.packs) > 1:
            # collect all files with this combination of packs
            files_with_same_packs = set()
            for file_path_other, file_info_other in all_files_dict.iteritems():
                if file_info_other.packs == file_info.packs:
                    files_with_same_packs.add(file_path_other)

            # generate new shared pack with name combined with merged files packs
            shared_pack_name = '_' + str(auto_pack_index).zfill(4)
            auto_pack_index += 1
            # no dependencies for auto generated new pack and pass set() for dependencies
            shared_pack = Pack(shared_pack_name, files_with_same_packs, [])
            all_packs.add(shared_pack)
            shared_pack.convert_dependencies_from_str_to_objects(all_packs)
            # save previous packs list to later use
            prev_packs = file_info.packs
            # update all_files_dict to move dependencies from old pack to new generated pack
            for f in files_with_same_packs:
                file_info_obj = all_files_dict[f]
                file_info_obj.packs = {shared_pack}  # depends only on one pack
            # update old packs - remove extracted files and add dependency to new shared pack
            for prev_pack in prev_packs:
                for extracted_file in files_with_same_packs:
                    prev_pack.files.remove(extracted_file)
                prev_pack.dependencies.add(shared_pack)  # extracted files now in dependent shared_pack


def load_excluded_files_list(excluded_json_file):
    if excluded_json_file:
        with open(excluded_json_file) as f:
            data = yaml.load(f)
            if not isinstance(data, list):
                raise ValueError("excluded json should contain list of files")
            # always use unix / in file path to compare strings values
            data = [path.replace('\\', '/') for path in data]
        return data
    else:
        return []


def is_texture(p):
    _, ext = os.path.splitext(p)
    return ext in ('.png', '.tga', '.psd', '.pvr', '.dds', '.webp')


def is_texture_descriptor(p):
    return p.endswith(".tex")


def has_texture_file(pack):
    for f in pack.files:
        if is_texture(f):
            return True
    return False


def has_texture_descriptor_file(pack):
    for f in pack.files:
        if is_texture_descriptor(f):
            return True
    return False


def extract_textures(files):
    textures = list(f for f in files if is_texture(f))

    for t in textures:
        files.remove(t)
    return textures


def extract_tex_descriptors(files):
    tex_descriptors = list(f for f in files if is_texture_descriptor(f))

    for t in tex_descriptors:
        files.remove(t)
    return tex_descriptors


def split_pack(pack, existing_packs):
    packs = []
    if has_texture_file(pack) and has_texture_descriptor_file(pack):
        files_count_before = len(pack.files)
        # generate sub-packs with extracted textures and then texture descriptors and other files stay in original pack
        # collect all textures
        textures_from_pack = extract_textures(pack.files)
        # collect all tex_descriptors
        tex_descriptors_from_pack = extract_tex_descriptors(pack.files)

        img_pack_name = pack.name + "_img"
        if any(pack.name == img_pack_name for pack in existing_packs):
            raise ValueError('Package name already used!: ' + str(img_pack_name))

        sub_textures_pack = Pack(img_pack_name, textures_from_pack, [])

        tex_pack_name = pack.name + "_tex_desc"
        if any(pack.name == tex_pack_name for pack in existing_packs):
            raise ValueError('Package name already used!: ' + str(tex_pack_name))

        sub_tex_descriptors_pack = Pack(tex_pack_name, tex_descriptors_from_pack,
                                        [sub_textures_pack.name])

        pack.dependencies.append(sub_tex_descriptors_pack.name)

        packs = [pack,                        # original pack
                 sub_tex_descriptors_pack,    # extracted texture descriptors pack
                 sub_textures_pack]           # extracted images pack
        # validate file changes
        assert (files_count_before == len(pack.files) + len(sub_tex_descriptors_pack.files) + len(sub_textures_pack.files))
    else:
        # just left original pack
        packs.append(pack)
    return packs


def load_all_descriptors(descriptors_list, excluded_files, input_data_dir):
    packs = set()  # all packs are unique objects, its name unique too
    for descriptor_file_name in descriptors_list:
        with open(descriptor_file_name) as f:
            pack_description_data = yaml.load(f)
        if not isinstance(pack_description_data, list):
            msg = "{} have had wrong format {}, expected list".format(descriptor_file_name,
                                                                      type(pack_description_data))
            raise TypeError(msg)
        for data in pack_description_data:
            pack = Pack(*Pack.parse_data(data, excluded_files, input_data_dir, packs))
            # split pack into dependent textures-descriptors-sprites for correct loading order
            packs_generated = split_pack(pack, packs)
            for sub_pack in packs_generated:
                packs.add(sub_pack)

    # now we can convert string dependencies names to list of objects
    convert_pack_dependencies_names_to_objects(packs)
    # check every pack dependencies successfully converted from list to set type
    assert(all(type(pack.dependencies) is set for pack in packs))
    return packs


def convert_pack_dependencies_names_to_objects(all_packs):
    for pack in all_packs:
        pack.convert_dependencies_from_str_to_objects(all_packs)


class Pack(object):
    def __init__(self, name, files, dependencies):
        self.name = name  # str name
        self.files = files  # list of relative path to file
        self.dependencies = dependencies  # set of dependency packs

    @staticmethod
    def parse_data(data, excluded_files, input_data_dir, existing_packs):
        pack_name = data.get("name")
        if not pack_name:
            raise ValueError("Package name is empty!")
        # all pack names are unique
        if any(pack.name == pack_name for pack in existing_packs):
            raise ValueError('Package name already used!: ' + str(pack_name))

        files_from_json = data.get("files", [])
        # always use unix / in file path to compare strings values
        correct_slash_files = [path.replace('\\', '/') for path in files_from_json]
        pack_files = filter(lambda f: f not in excluded_files, correct_slash_files)

        # collect all files from folders in json pack description
        # filter with excluded_files
        folders_from_json = data.get("folders", [])
        for folder in folders_from_json:
            for root, dirs, files in os.walk(os.path.join(input_data_dir, folder)):
                for file_name in files:
                    file_path = os.path.join(root, file_name)
                    file_path_from_folder = os.path.normpath(os.path.relpath(file_path, input_data_dir))
                    if file_path_from_folder not in excluded_files:
                        # always use unix / in file path to compare strings values
                        pack_files.append(file_path_from_folder.replace('\\', '/'))

        dependencies = data.get("dependencies", [])

        return pack_name, pack_files, dependencies

    def add_file(self, file_name):
        self.files.add(file_name.replace('\\', '/'))

    def add_dependency(self, dependency):
        self.dependencies.append(dependency)

    # call only once on loading data from json
    def convert_dependencies_from_str_to_objects(self, all_packs):
        dependencies = set()
        for dep in self.dependencies:
            for pack in all_packs:
                if pack.name == dep:
                    dependencies.add(pack)
                    break
        # all dependencies found
        assert(len(self.dependencies) == len(dependencies))
        self.dependencies = dependencies


def verify_file_exists(file_path):
    dir_path, file_name = os.path.split(os.path.normpath(file_path))
    if file_name not in os.listdir(dir_path):
        msg = "File {} does not exist in folder: {}".format(file_name, dir_path)
        raise LookupError(msg)


def build_database(data_dir, packs, output_path):
    os.remove(output_path) if os.path.exists(output_path) else None

    with sqlite3.connect(output_path) as connection:
        cur = connection.cursor()

        cur.execute("CREATE TABLE IF NOT EXISTS files (path TEXT PRIMARY KEY, pack_index INTEGER NOT NULL);")
        cur.execute("CREATE TABLE IF NOT EXISTS packs "
                    "(\"index\" INTEGER PRIMARY KEY, name TEXT UNIQUE, dependency TEXT NOT NULL);")

        packs_sorted_list = sorted(packs, key=lambda pack_obj: pack_obj.name)

        for pack_index, pack in enumerate(packs_sorted_list):
            files_sorted_list = sorted(pack.files, key=lambda file_name: file_name)
            for rel_file_path in files_sorted_list:
                path = os.path.join(data_dir, rel_file_path)
                verify_file_exists(path)
                # always use unix slashes in names it works everywhere
                rel_file_path = rel_file_path.replace('\\', '/')
                try:
                    cur.execute("INSERT INTO files VALUES (?, ?)", (rel_file_path, pack_index))
                except Exception:
                    msg = "error: INSERT INTO files VALUES ({}, {}) #  pack_name: {}  failed.\n".format(rel_file_path,
                                                                                                        str(pack_index),
                                                                                                        pack.name)
                    sys.stderr.write(msg)
                    cur.execute("SELECT * FROM files WHERE path=(?)", (rel_file_path,))
                    line = cur.fetchall()[0]
                    other_pack_index = line[1]
                    sys.stderr.write('cause: (' + rel_file_path + ', ' + str(other_pack_index)
                                     + ') #  pack_name: ' + packs_sorted_list[other_pack_index].name +
                                     ' already exist\n')
                    raise
            indices_str = ", ".join(map(str, [packs_sorted_list.index(dependency) for dependency in pack.dependencies]))
            try:
                cur.execute("INSERT INTO packs VALUES (?, ?, ?)", (pack_index, pack.name, indices_str))
            except Exception:
                msg = "INSERT INTO packs VALUES ({}, {}, {})\n".format(str(pack_index), str(pack.name),
                                                                       str(indices_str))
                sys.stderr.write(msg)
                raise
        connection.commit()


def call_archiver(resource_archiver_path, base_dir, db_path, dummy_file_data, out_pack_file):
    cmd = [os.path.normpath(resource_archiver_path), 'pack', '-metadb', db_path]

    if dummy_file_data:
        cmd.extend([
            '-basedir', base_dir + '/',
            '-dummyFileData'])
    else:
        cmd.extend([
            '-basedir', base_dir + '/'])

    cmd.extend([out_pack_file])

    subprocess.check_call(cmd, stderr=subprocess.STDOUT)


if __name__ == '__main__':
    arguments = parse_args()
    if arguments.output_dvpl_dir:
        raise NotImplementedError("TODO implement it (unpack from dvpk)")
    elif arguments.output_dvpk:
        try:
            input_files_list = arguments.input_files[0].split(' ')
            generate_dvpk(arguments.resource_archiver, arguments.input_data_dir, input_files_list,
                          arguments.input_excluded, arguments.output_dvpk, arguments.output_meta,
                          arguments.dummyFileData)
        except Exception:
            sys.stderr.write('error: failed to generate dvpk\n')
            raise
    elif arguments.output_files_dir:
        raise NotImplementedError("TODO implement it (copy files to output dir)")
    elif arguments.output_meta:
        #  generate only meta
        generate_meta(arguments.input_data_dir, arguments.input_files_list, arguments.input_excluded,
                      arguments.output_meta)
    else:
        sys.stderr.write('error: check input parameters\n')
        exit(3)
