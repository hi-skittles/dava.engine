import sys
import subprocess
import shutil
import os

def copy_recursively(source_folder, destination_folder):
	source_folder = os.path.abspath(source_folder)
	destination_folder = os.path.abspath(destination_folder)

	for root, dirs, files in os.walk(source_folder):
	    for item in files:
	        src_path = os.path.join(root, item)
	        dst_path = destination_folder + src_path.replace(source_folder, "")
	        if os.path.exists(dst_path):
	            if os.stat(src_path).st_mtime > os.stat(dst_path).st_mtime:
	                shutil.copy2(src_path, dst_path)
	        else:
	            shutil.copy2(src_path, dst_path)
	    for item in dirs:
	        src_path = os.path.join(root, item)
	        dst_path = destination_folder + src_path.replace(source_folder, "")
	        if not os.path.exists(dst_path):
	            os.mkdir(dst_path)

def process_template(in_file_path, out_file_path, res_editor_path):

	in_file = open(os.path.abspath(in_file_path), "r")
	out_file = open(os.path.abspath(out_file_path), "w")

	for line in in_file:

		if line.find("@RES_EDITOR_PATH@") != -1:
			line = line.replace("@RES_EDITOR_PATH@", res_editor_path)

		if line.find("@RES_EDITOR_BINARY@") != -1:
			line = line.replace("@RES_EDITOR_BINARY@", res_editor_path + "/ResourceEditor")

		out_file.write(line)

	in_file.close()
	out_file.close()

def copy_scripts():

	davaConfig = open("../../../DavaConfig.in", "r")

	for line in davaConfig:
		if line.find("RES_EDITOR_PATH") != -1:
			res_editor_path = line.split("=")[1].replace(" ", "")

	davaConfig.close()

	if 'res_editor_path' in locals():

		process_template("../scripts/TemplateConvert3D.in", "../DataSource/convert_3d.py", res_editor_path)
		process_template("../scripts/TemplateConvert3D_FX.in", "../DataSource/convert_3d_FX.py", res_editor_path)
		process_template("../scripts/TemplateConvert3DTanks.in", "../DataSource/convert_3d_tanks.py", res_editor_path)

def copy_data():
	
	os.chdir("../Data")
	os.system("git clean -dxf")

	os.chdir("../DataSource")
	os.system("git clean -dxf")

	os.system("mkdir ../Data/Materials")
	os.system("mkdir ../Data/Shaders")

	copy_recursively("../../../../performance.test/Data", "../Data")
	copy_recursively("../../../Programs/ResourceEditor/Data/Materials", "../Data/Materials")
	copy_recursively("../../../Programs/ResourceEditor/Data/Shaders", "../Data/Shaders")
	copy_recursively("../../../../performance.test/Data", "../Data")

	copy_recursively("../../../../performance.test/DataSource", "../DataSource")

	copy_scripts()

	convert_everything = [sys.executable, "convert_everything.py"]
	subprocess.call(convert_everything)

if __name__ == '__main__':
    copy_data()
