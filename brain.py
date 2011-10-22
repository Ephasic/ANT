import requests
import string
import re
import urllib2
import sys

SN = "SEND_NOTHING"

def remove_html_tags(data):
    p = re.compile(r'<.*?>')
    return p.sub('', data)

def remove_bracket_tags(data):
    p = re.compile(r'\[[^]]+]')
    return p.sub('', data)

def brain(raw_query = "c plus plus"):
	query = urllib2.quote(raw_query)
	r = requests.get('http://en.wikipedia.org/wiki/Special:Export?search='+query+'&go=Go')
	html_content = r.content
	first_paragraph = ""
	for i in range(html_content.index("<p>"),html_content.index("</p>")):
		first_paragraph += html_content[i]
	return remove_bracket_tags(remove_html_tags(first_paragraph))

def stripFromEnd(victim,x):
	toReturn = ""
	for i in range(0,len(victim)-x):
		toReturn += victim[i]
	return toReturn

recieved = ""
query = ""
failedSearch = "For search options, see Help:Searching."
ambiguity = "may refer to:"

for i in range(1,len(sys.argv)):
	recieved += str(sys.argv[i]+" ")

query = urllib2.quote(recieved)
r = requests.get('http://en.wikipedia.org/w/index.php?title='+query+'&action=render')
html_content = r.content
first_paragraph = ""
for i in range(html_content.index("<p>"),html_content.index("</p>")):
	first_paragraph += html_content[i]
print remove_bracket_tags(remove_html_tags(first_paragraph))
