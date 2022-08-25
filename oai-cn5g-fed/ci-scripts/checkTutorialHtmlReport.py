#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *   http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *   contact@openairinterface.org
# */
#---------------------------------------------------------------------

import os
import re
import sys
import subprocess

class HtmlReport():
	def __init__(self):
		self.job_name = ''
		self.job_id = ''
		self.job_url = ''
		self.job_start_time = 'TEMPLATE_TIME'
		self.amfConfigStatus = True
		self.smfConfigStatus = True
		self.upfConfigStatus = True

	def generate(self):
		cwd = os.getcwd()
		self.file = open(cwd + '/test_results_oai_cn5g_tutorials.html', 'w')
		self.generateHeader()

		tutorials = ['mini-gnbsim', 'static-ue-ip', 'vpp-upf-gnbsim', 'slicing-with-nssf']
		for tutorial in tutorials:
			if not os.path.isfile(cwd + '/archives/' + tutorial + '.log'):
				continue
			if not os.path.isdir(cwd + '/archives/' + tutorial):
				continue
			self.tutorialSummary(tutorial)

		self.generateFooter()
		self.file.close()

	def generateHeader(self):
		# HTML Header
		self.file.write('<!DOCTYPE html>\n')
		self.file.write('<html class="no-js" lang="en-US">\n')
		self.file.write('<head>\n')
		self.file.write('  <meta name="viewport" content="width=device-width, initial-scale=1">\n')
		self.file.write('  <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">\n')
		self.file.write('  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>\n')
		self.file.write('  <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>\n')
		self.file.write('  <title>OAI Core Network Test Results for ' + self.job_name + ' job build #' + self.job_id + '</title>\n')
		self.file.write('</head>\n')
		self.file.write('<body><div class="container">\n')
		self.file.write('  <table width = "100%" style="border-collapse: collapse; border: none;">\n')
		self.file.write('   <tr style="border-collapse: collapse; border: none;">\n')
		self.file.write('	<td style="border-collapse: collapse; border: none;">\n')
		self.file.write('	  <a href="http://www.openairinterface.org/">\n')
		self.file.write('		 <img src="http://www.openairinterface.org/wp-content/uploads/2016/03/cropped-oai_final_logo2.png" alt="" border="none" height=50 width=150>\n')
		self.file.write('		 </img>\n')
		self.file.write('	  </a>\n')
		self.file.write('	</td>\n')
		self.file.write('	<td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
		self.file.write('	  <b><font size = "6">Job Summary -- Job: ' + self.job_name + ' -- Build-ID: <a href="' + self.job_url + '">' + self.job_id + '</a></font></b>\n')
		self.file.write('	</td>\n')
		self.file.write('   </tr>\n')
		self.file.write('  </table>\n')
		self.file.write('  <br>\n')

	def generateFooter(self):
		self.file.write('  <div class="well well-lg">End of Test Report -- Copyright <span class="glyphicon glyphicon-copyright-mark"></span> 2022 <a href="http://www.openairinterface.org/">OpenAirInterface</a>. All Rights Reserved.</div>\n')
		self.file.write('</div></body>\n')
		self.file.write('</html>\n')

	def tutorialSummary(self, tutorial):
		cwd = os.getcwd()
		tutoName = ''
		tutoStatus = True
		cmdSummary = False
		listOfCmds = []
		with open(cwd + '/archives/' + tutorial + '.log','r') as tutoLog:
			for line in tutoLog:
				result = re.search('Final result for the tutorial (?P<doc_name>[a-zA-Z0-9\.\_:]+)', line)
				if result is not None:
					tutoName = result.group('doc_name')
					if re.search('FAIL', line) is not None:
						tutoStatus = False
				if cmdSummary:
					if re.search('FAIL', line) is not None:
						cmd = re.sub(': FAIL.*$', '', line.strip())
						cmdStatus = False
					else:
						cmd = re.sub(': PASS.*$', '', line.strip())
						cmdStatus = True
					listOfCmds.append((cmd,cmdStatus))
				result = re.search('Command execution summary', line)
				if result is not None:
					cmdSummary = True
		tutoLog.close()

		log_files = sorted(os.listdir(cwd + '/archives/' + tutorial))
		deployedContainerImages = []
		for log_file in log_files:
			if not log_file.endswith(".log"):
				continue
			if re.search('gnbsim', log_file) is not None or \
			   re.search('rfsim5g-oai', log_file) is not None or \
			   re.search('ueransim', log_file) is not None:
				continue
			rootName = re.sub('.log.*$', '', log_file)
			containerName = 'oai-' + rootName
			if re.search('spgwu', rootName) is not None:
				imageRootName = 'oai-spgwu-tiny:'
				fileRootName = re.sub('-slice.*','',rootName)
			elif re.search('vpp-upf', rootName) is not None:
				imageRootName = 'oai-upf-vpp:'
				fileRootName = 'upf_vpp'
				containerName = rootName
			else:
				imageRootName = 'oai-' + re.sub('-slice.*','',rootName) + ':'
				fileRootName = re.sub('-slice.*','',rootName)
			imageTag = ''
			imageSize = ''
			imageDate = ''
			with open(cwd + '/archives/oai_' + fileRootName + '_image_info.log','r') as imageDetailsFile:
				for line in imageDetailsFile:
					result = re.search('TAG: oai-.*:(?P<tag>[a-zA-Z0-9\-\_]+)', line)
					if result is not None:
						imageTag = result.group('tag')
					result = re.search('Size = (?P<size>[0-9]+) bytes', line)
					if result is not None:
						sizeInt = int(result.group('size'))
						if sizeInt < 1000000:
							sizeInt = int(sizeInt / 1000)
							imageSize = str(sizeInt) + ' kB'
						else:
							sizeInt = int(sizeInt / 1000000)
							imageSize = str(sizeInt) + ' MB'
					result = re.search('Date = (?P<date>[a-zA-Z0-9\-\_:]+)', line)
					if result is not None:
						imageDate = re.sub('T', '  ', result.group('date'))
			imageDetailsFile.close()
			deployedContainerImages.append((containerName, imageRootName + imageTag, imageSize, imageDate))

		if tutoName == '':
			return
		self.file.write('  <h2>Tutorial Check Summary </h2>\n')

		if tutoStatus:
				self.file.write('  <div class="alert alert-success">\n')
				self.file.write('	<strong>Successful Check for ' + tutoName + '! <span class="glyphicon glyphicon-warning-sign"></span></strong>\n')
				self.file.write('  </div>\n')
		else:
				self.file.write('  <div class="alert alert-danger">\n')
				self.file.write('	<strong>Failed Check ' + tutoName + '! <span class="glyphicon glyphicon-warning-sign"></span></strong>\n')
				self.file.write('  </div>\n')

		self.file.write('  <br>\n')
		self.file.write('  <button data-toggle="collapse" data-target="#' + tutorial + '-details">More details on tutorial results</button>\n')
		self.file.write('  <br>\n')
		self.file.write('  <div id="' + tutorial + '-details" class="collapse">\n')
		self.file.write('  <table class="table-bordered" width = "100%" align = "center" border = 1>\n')
		self.file.write('	 <tr bgcolor = "#33CCFF" >\n')
		self.file.write('	   <th>Container Name</th>\n')
		self.file.write('	   <th>Used Image Tag</th>\n')
		self.file.write('	   <th>Image Creation Date</th>\n')
		self.file.write('	   <th>Used Image Size</th>\n')
		self.file.write('	 </tr>\n')
		for (cName,iTag,iDate,iSize) in deployedContainerImages:
			self.file.write('	 <tr>\n')
			self.file.write('	   <td>' + cName + '</td>\n')
			self.file.write('	   <td>' + iTag + '</td>\n')
			self.file.write('	   <td>' + iDate + '</td>\n')
			self.file.write('	   <td>' + iSize + '</td>\n')
			self.file.write('	 </tr>\n')
		self.file.write('  </table>\n')
		self.file.write('  <table class="table-bordered" width = "100%" align = "center" border = 1>\n')
		self.file.write('	 <tr bgcolor = "#33CCFF" >\n')
		self.file.write('	   <th>Command</th>\n')
		self.file.write('	   <th>Status</th>\n')
		self.file.write('	 </tr>\n')
		for (cmd,cmdStatus) in listOfCmds:
			self.file.write('    <tr>\n')
			self.file.write('	   <td>'+ cmd + '</td>\n')
			if cmdStatus:
				self.file.write('	   <td bgcolor="lightgreen"><b>PASS</b></td>\n')
			else:
				self.file.write('	   <td bgcolor="lightcoral"><b>FAIL</b></td>\n')
			self.file.write('    </tr>\n')
		self.file.write('  </table>\n')
		self.file.write('  </div>\n')
		self.file.write('  <br>\n')

def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('checkTutorialHtmlReport.py')
	print('   Generate an HTML report for the Jenkins pipeline on oai-cn5g-fed.')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python3 checkTutorialHtmlReport.py [options]')
	print('  --help  Show this help.')
	print('---------------------------------------------------------------------------------------------- Mandatory Options -----')
	print('  --job_name=[Jenkins Job name]')
	print('  --job_id=[Jenkins Job Build ID]')
	print('  --job_url=[Jenkins Job Build URL]')

#--------------------------------------------------------------------------------------------------------
#
# Start of main
#
#--------------------------------------------------------------------------------------------------------

argvs = sys.argv
argc = len(argvs)

HTML = HtmlReport()

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-job_name=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_name=(.+)$', myArgv, re.IGNORECASE)
		HTML.job_name = matchReg.group(1)
	elif re.match('^\-\-job_id=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_id=(.+)$', myArgv, re.IGNORECASE)
		HTML.job_id = matchReg.group(1)
	elif re.match('^\-\-job_url=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_url=(.+)$', myArgv, re.IGNORECASE)
		HTML.job_url = matchReg.group(1)
	else:
		sys.exit('Invalid Parameter: ' + myArgv)

if HTML.job_name == '' or HTML.job_id == '' or HTML.job_url == '':
	sys.exit('Missing Parameter in job description')

HTML.generate()
