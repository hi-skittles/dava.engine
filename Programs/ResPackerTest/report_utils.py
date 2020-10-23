import re

def parse_error(txt, regular):
	p = re.compile(regular)
	return p.findall(txt)

# TEXT REPORT
def print_bool(val):
	if val:
		return "PASSED"
	return "FAILURE"

def print_result(result):
	print "************************************************************"
	print "*** Test results"
	print "************************************************************"
	print "*** Tests run %d Test Success %d Test Fails %d" % (result['tests'], result['success'], result['tests'] - result['success'])
	print "*** Failures: Txt %d Tex %d Image %d" % (result['txt_failure'], result['tex_failure'], result['img_failure'])
	print "************************************************************"
	
	print "   %-60s%10s%10s%10s%10s" % ("Test name", "Result", "Text", "Tex", "Image")
	for test in result["Tests"].values():
		print " - %-60s%10s%10s%10s%10s" % (test["Name"], print_bool(test["Success"]), print_bool(test["txt_Success"]), print_bool(test["tex_Success"]), print_bool(test["img_Success"]))

def log_error(message):
	message = message.replace("|", "||")

	message = message.replace("'", "|'")
	message = message.replace("\n", "|n")
	message = message.replace("\r", "|r")

	message = message.replace("\u0085", "|x")
	message = message.replace("\u2028", "|l")
	message = message.replace("\u2029", "|p")

	message = message.replace("[", "|[")
	message = message.replace("]", "|]")

	print "##teamcity[message text='" + message + "' errorDetails='' status='ERROR']"
		
# HTML REPORT
report = None

def create_html(result, filename):
	global report
	report = open(filename, 'w')
	if report == None:
		log_error("Couldn't open file for reocring " + filename)
		return
	
	create_header("<h1> Resource Packer autotest results for %s</h1>" % (result['gpu']))
	report.write('<a href="%s/%s.zip" target="_blank"> %s.zip </a>' % ('Artifacts/',result['gpu'],result['gpu']))
	report.write('<br>')
	report.write('<br>')
	report.write('<br>')
	# Create Summary
	open_table()
	open_row()
	write_cell("GPU")
	write_cell("Test run")
	write_cell("Test Passed")
	write_cell("Test Failed")
	write_cell("Text")
	write_cell("Tex")
	write_cell("Image")
	close_row()
	open_row()
	write_cell(result['gpu'])
	write_cell(result['tests'])
	write_cell(result["success"])
	write_cell(result["tests"] - result["success"])
	write_cell(result["txt_failure"])
	write_cell(result["tex_failure"])
	write_cell(result["img_failure"])
	close_row()
	close_table()
	report.write('<br>')
	report.write('<br>')
	# Create test details
	open_table()
	open_row()
	write_cell("Test Name")
	write_cell("Test Result")
	write_cell("TEX")
	write_cell("Text")
	write_cell("Image")
	write_cell("Error")
	close_row()
	for test in result["Tests"].values():
		open_row()
		write_cell(test['Name'])
		if test["img_Success"]:
			write_cell_result(test['Success'])
			write_cell_result(test["tex_Success"])
			write_cell_result(test["txt_Success"])
			write_cell_result(test["img_Success"])
			write_cell(test['Error_msg'])
		else:
			names = parse_error(test['Error_msg'], "\S+/(\S+png)")
			open_cell()
			for name in names:
				insert_image('./Artifacts/' + result['gpu'], test['Name'], name)
			close_cell()
			write_cell_result(test["tex_Success"])
			write_cell_result(test["txt_Success"])
			open_cell()
			for name in names:
				insert_image('./Artifacts/' + result['gpu'] , test['Name']+ '/expected_results', name)
			close_cell()
			write_cell(test['Error_msg'])
			for name in names:
				open_cell()
				insert_image('./Artifacts/' + result['gpu'] , test['Name'], name + '-diff1.png')
				insert_image('./Artifacts/' + result['gpu'] , test['Name'], name + '-diff2.png')
				close_cell()
		close_row()
	close_table()
	#report.write('<table style="border: 1px solid; cellspacing: 0px; cellpadding: 0px; border-collapse: collapse;">')
	
	report.close()
	
def create_header(txt):
	global report
	if None != report:
		report.write("<h1> %s </h1>\n" % (txt))
		
def open_table():
	global report
	if None != report:
		report.write('<table style="border: 1px solid; cellspacing: 0px; cellpadding: 0px; border-collapse: collapse;">\n')

def open_row():
	global report
	if None != report:
		report.write('<tr style="border: 1px solid; cellspacing: 0px; cellpadding: 0px;">\n')

def write_cell(txt):
	txt = str(txt)
	txt = txt.replace("\n", "<br>")

	global report
	if None != report:      
		report.write('<td style="border: 1px solid; cellspacing: 0px; cellpadding: 0px;"> %s </td>\n' % txt)
		
def write_cell_result(txt):
	global report
	if None != report:
		if txt == True:
			report.write('<td style="border: 1px solid; cellspacing: 0px; cellpadding: 0px; color: green"> PASSED </td>\n')
		else:
			report.write('<td style="border: 1px solid; cellspacing: 0px; cellpadding: 0px; color: red"> FAILED </td>\n')

def insert_image(path, test, file):
	global report
	if None != report:  
		report.write('<a href="%s/%s/%s" target="_blank">' % (path, test, file))
		report.write('<img src="%s/%s/%s" alt="%s" style="height: 60px;" >\n' % (path, test, file, test))
	
def open_cell():
	global report
	if None != report:
		report.write('<td style="border: 1px solid; cellspacing: 0px; cellpadding: 0px; color: green">\n')
	
def close_cell():
	global report
	if None != report:
		report.write('</td>\n')
	
def close_table():
	global report
	if None != report:
		report.write('</table>\n')

def close_row():
	global report
	if None != report:
		report.write('</tr>\n')