import os;
import os.path;
import report_utils;
import platform;
import utils;
import shutil;
import subprocess;
import sys;
import zipfile;
	
	
arguments = sys.argv[1:]

if 3 > len(arguments):
	print 'Usage: ./create_performance_mail_report.py Gpu Recepients Link [Branch Revision]'
	exit(1)

gpu = arguments[0]  
recipients = arguments[1]
link = arguments[2]
branch = 0
revision = 0

if 5 == len(arguments):
	branch = arguments[3]
	revision = arguments[4]

currentDir = os.getcwd();

toolDir = os.path.realpath(currentDir + "/../../Tools/Bin/")
data = os.path.realpath(currentDir + "/DataSource/")
input = os.path.realpath(currentDir + "/DataSource/TestData/")
output =  os.path.realpath(currentDir + "/Data/TestData/")
data_folder =  os.path.realpath(currentDir + "/Data")
process = os.path.realpath(currentDir + "/DataSource/$process/")
results = os.path.realpath(currentDir + "/Results/" + gpu)

tests_results = {"Tests" : {}}

print "*** DAVA AUTOTEST Cleen up working dirctories ***"

if os.path.exists(currentDir + "/Artifacts/" + gpu):
	print "Remove folder " + currentDir + "/Artifacts/" + gpu
	shutil.rmtree(currentDir + "/Artifacts/" + gpu)
	
if os.path.exists(output):
	print "Remove folder " + output
	shutil.rmtree(output)

if os.path.exists(process):
	print "Remove folder " + process
	shutil.rmtree(process)
	
if not os.path.exists(data_folder):
	print "Create folder " + data_folder
	os.mkdir(data_folder)

print "*** DAVA AUTOTEST Run convert_graphics.py script for %s ***" % gpu
os.chdir(data)

params = [sys.executable, 'convert_graphics.py']
if (len(arguments) > 0):
	params = params + ["-gpu", arguments[0]]
	
print "subprocess.call " + "[%s]" % ", ".join(map(str, params))
f = open(gpu + "_log.txt", 'w')
subprocess.call(params, stdout=f)
f.close()

shutil.move(gpu + "_log.txt", output)

print "*** DAVA AUTOTEST Check result for %s ***" % gpu
os.chdir(currentDir)

os_name = "Windows"
print "Convert DDS files:"
if (platform.system() == "Windows"):
	subprocess.call(toolDir + "/ImageUnpacker.exe -folder " + output, shell=True)
else:
	os_name = "MacOS"
	subprocess.call(toolDir + "/ImageUnpacker -folder " + output, shell=True)

i = 0

for test in os.listdir(results):
	if(os.path.isdir(os.path.realpath(results + "/" + test))):
		i = i + 1
		result = {}
		print "*** Test#%d %s:" % (i, test)
		
		result['Name'] = test
		result['Number'] = i
		result['Success'] = True
		result['Error_msg'] = ""
		result['txt_Success'] = True
		result['tex_Success'] = True
		result['img_Success'] = True
		
		expected = os.path.realpath(results + "/" + test)
		actual = os.path.realpath(output + "/" + test)
		
		# Check TEXT files
		print "Check TXT files"
		files = filter(lambda x: x[-3:] == "txt", os.listdir(expected))
		if len(files) != 0:
			print files
		
		for file in files:
			res = utils.compare_txt(expected + "/" + file, actual + "/" + file)
			if res != None:
				result['txt_Success'] = False
				result['Error_msg'] = result['Error_msg'] + str(res) + "\n"
				print res
			
		#Check TEX files
		print "Check TEX files"
		files = filter(lambda x: x[-3:] == "tex", os.listdir(expected))
		if len(files) != 0:
			print files
		
		for file in files:
			res = utils.compare_tex(expected + "/" + file, actual + "/" + file)
			if res != None:
				result['tex_Success'] = False
				result['Error_msg'] = result['Error_msg'] + str(res) + "\n"
				print res
		
		# Check IMAGE files
		print "Check IMAGE files"
		files = filter(lambda x: x[-3:] == "png", os.listdir(expected))
		if len(files) != 0:
			print files
		
		for file in files:
			res = utils.compare_img(expected + "/" + file, actual + "/" + file)
			if isinstance(res, str):
				result['img_Success'] = False
				result['Error_msg'] = result['Error_msg'] + str(res) + "\n"
			else:
				if res > 0.01:
					result['img_Success'] = False
					result['Error_msg'] = result['Error_msg'] + "Image %s differce from expected on %f%%.\n" % (actual + "/" + file, res * 100)
					utils.save_diff(expected + "/" + file, actual + "/" + file)
		
		
		result['Success'] = result['tex_Success'] and result['txt_Success'] and result['img_Success']
		
		if result['Success']:
			print "Test passed!"
		tests_results["Tests"][test] = result
		
		#Check graphics files
		print
		print

# Make final results
test_num = 0
test_success = 0
tex_failure = 0
txt_failure = 0
img_failure = 0

for test in tests_results["Tests"].values():
	test_num = test_num + 1
		
	if test['Success']:
		test_success = test_success + 1
	
	if not test['tex_Success']:
		tex_failure = tex_failure + 1
		
	if not test['txt_Success']:
		txt_failure = txt_failure + 1
	
	if not test['img_Success']:
		img_failure = img_failure + 1

tests_results['tests'] = i
tests_results["success"] = test_success
tests_results["tex_failure"] = tex_failure
tests_results["txt_failure"] = txt_failure
tests_results["img_failure"] = img_failure
tests_results['gpu'] = gpu
	
report_utils.print_result(tests_results)
report_utils.create_html(tests_results, currentDir + "/" + gpu + ".html")

print
print

if tests_results["success"] != tests_results['tests']:
	print "*** DAVA AUTOTEST Send letter with info about failures ***"
	subject = "[AUTOTEST] Test for resource packer: Platform = %s GPU = %s" % (os_name, gpu)
	msg = "Test: runned= %d succes= %d failed= %d <br>" % (tests_results['tests'], tests_results['success'], tests_results['tests'] - tests_results['success'])
	msg += "Failures: Txt %d Tex %d Image %d <br>" % (tests_results['txt_failure'], tests_results['tex_failure'], tests_results['img_failure'])
	msg += "<br> Link: %s/%s.html" % (link, gpu)
	if (branch != 0):
		msg += "<br>Framewok: %s %s" % (branch, revision)
	
	utils.call("python", "mail.py", recipients, subject, msg)
	print
	print
	
	
print "*** DAVA AUTOTEST Copy results for artifact storing ***"
print "Copy results for storing in TC %s -> %s" % (output, currentDir + "/Artifacts/" + gpu)
shutil.copytree(output, currentDir + "/Artifacts/" + gpu)
for test in os.listdir(results):
	shutil.copytree(input + "/" + test, currentDir + "/Artifacts/" + gpu + "/" + test + "/input/")
	
	shutil.copytree(results + "/" + test, currentDir + "/Artifacts/" + gpu + "/" + test + "/expected_results/")

print "*** DAVA AUTOTEST Zip results for artefacts ***"
os.chdir(currentDir + "/Artifacts/")
utils.zip(gpu, gpu)
