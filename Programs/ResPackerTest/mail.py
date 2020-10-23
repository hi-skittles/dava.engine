import smtplib
import sys
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText

arguments = sys.argv[1:]

if len(arguments) != 3:
    print 'Usage: ./mail.py Recepient Subject Text'
    exit(1)

toaddr = arguments[0]
sbj = arguments[1]
link = arguments[2]
	
fromaddr = "WGMobileTeamCity@gmail.com"

# Compose mail
msg = MIMEMultipart()
msg['From'] = fromaddr
msg['To'] = toaddr
msg['Subject'] = sbj

msg.attach(MIMEText(link, 'html'))

server = smtplib.SMTP('smtp.gmail.com', 587)
 
#Next, log in to the server
server.ehlo()
server.starttls()
server.ehlo()
server.login(fromaddr, "78007800")

#Send the mail
text = msg.as_string()
server.sendmail(fromaddr, toaddr.split(','), text)
server.quit()