// Node.js uploader for bitbucket
// Ported from: https://bitbucket.org/Swyter/bitbucket-curl-upload-to-repo-downloads/

var execSync = require('child_process').execSync;
var fs       = require('fs');

if (process.argv.length < 6) {
	consooe.log("error: please specify user_name, password, repository, and file_to_upload");
	process.exit(1);
}

var user = process.argv[2];
var pass = process.argv[3];
var repo = process.argv[4];
var file = process.argv[5];

var output;
var csrf;

// get CSRF token from sign-in page
console.log("getting initial csrf token from the sign-in page:");
output = execSync("curl -k -c cookies.txt --progress-bar -o /dev/null https://bitbucket.org/account/signin/");
output = findLineWithText("csrf", fs.readFileSync("cookies.txt").toString());
csrf = output.toString().split("\t")[6];
if (csrf[csrf.length-1] === "\r" || csrf[csrf.length-1] === "\n") csrf = csrf.substring(0, csrf.length-1);

// sign in and get updated CSRF token
console.log("signing in with the credentials provided:");
output = execSync("curl -k -c cookies.txt -b cookies.txt --progress-bar -o /dev/null -d \"username="+user+"&password="+pass+"&submit=&next="+repo+"&csrfmiddlewaretoken="+csrf+"\" --referer \"https://bitbucket.org/account/signin/\" -L https://bitbucket.org/account/signin/");
output = findLineWithText("csrf", fs.readFileSync("cookies.txt").toString());
csrf = output.toString().split("\t")[6];
if (csrf[csrf.length-1] === "\r" || csrf[csrf.length-1] === "\n") csrf = csrf.substring(0, csrf.length-1);

// validate the credentials (otherwise exit before attempting to upload)
if (findLineWithText("bb_session", fs.readFileSync("cookies.txt").toString()) === "") {
	console.log("error: didn't get the session cookie, probably bad credentials or they changed stuff... upload canceled!");
	process.exit(1);
}

// upload file to the specified repository
console.log("actual upload progress should appear right now as a progress bar, be patient:");
output = execSync("curl -k -c cookies.txt -b cookies.txt --progress-bar -o /dev/null --referer \"https://bitbucket.org/"+repo+"/downloads\" -L --form csrfmiddlewaretoken="+csrf+" --form token= --form files=@\""+file+"\" https://bitbucket.org/"+repo+"/downloads");

// sign out and close session
console.log("done? maybe. *crosses fingers* signing out, closing session!");
output = execSync("curl -k -c cookies.txt -b cookies.txt --progress-bar -o /dev/null -L https://bitbucket.org/account/signout/");



function findLineWithText(searchText, allText) {
	var i;
	var lines = allText.split("\n");
	for (i=0; i<lines.length; i++) {
		if (lines[i].indexOf(searchText) >= 0)
			return lines[i];
	}
	return "";
}
