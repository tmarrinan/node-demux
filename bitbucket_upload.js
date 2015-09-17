// Node.js uploader for bitbucket
// Ported from: https://bitbucket.org/Swyter/bitbucket-curl-upload-to-repo-downloads/

var exec = require('child_process').exec;
var fs   = require('fs');

if (process.argv.length < 6) {
	console.log("error: please specify user_name, password, repository, and file_to_upload");
	process.exit(1);
}

var user = process.argv[2];
var pass = process.argv[3];
var repo = process.argv[4];
var file = process.argv[5];

function startUpload() {
	// get CSRF token from sign-in page
	console.log("getting initial csrf token from the sign-in page:");
	exec("curl -k -c cookies.txt --progress-bar -o NUL https://bitbucket.org/account/signin/", tokenCallback);
}

function tokenCallback(error, stdout, stderr) {
	if (error) throw error;

	console.log("########## 100%");

	var line = findLineWithText("csrf", fs.readFileSync("cookies.txt").toString());
	var csrf = line.toString().split("\t")[6];
	if (csrf[csrf.length-1] === "\r" || csrf[csrf.length-1] === "\n") csrf = csrf.substring(0, csrf.length-1);

	// sign in and get updated CSRF token
	console.log("signing in with the credentials provided:");
	exec("curl -k -c cookies.txt -b cookies.txt --progress-bar -o NUL -d \"username="+user+"&password="+pass+"&submit=&next="+repo+"&csrfmiddlewaretoken="+csrf+"\" --referer \"https://bitbucket.org/account/signin/\" -L https://bitbucket.org/account/signin/", signInCallback);
}

function signInCallback(error, stdout, stderr) {
	if (error) throw error;

	console.log("########## 100%");

	var line = findLineWithText("csrf", fs.readFileSync("cookies.txt").toString());
	var csrf = line.toString().split("\t")[6];
	if (csrf[csrf.length-1] === "\r" || csrf[csrf.length-1] === "\n") csrf = csrf.substring(0, csrf.length-1);

	// validate the credentials (otherwise exit before attempting to upload)
	if (findLineWithText("bb_session", fs.readFileSync("cookies.txt").toString()) === "") {
		console.log("error: didn't get the session cookie, probably bad credentials or they changed stuff... upload canceled!");
		process.exit(1);
	}

	// upload file to the specified repository
	console.log("actual upload progress should appear right now as a progress bar, be patient:");
	exec("curl -k -c cookies.txt -b cookies.txt --progress-bar -o NUL --referer \"https://bitbucket.org/"+repo+"/downloads\" -L --form csrfmiddlewaretoken="+csrf+" --form token= --form files=@\""+file+"\" https://bitbucket.org/"+repo+"/downloads", uploadCallback);
}

function uploadCallback(error, stdout, stderr) {
	if (error) throw error;

	console.log("########## 100%");

	// sign out and close session
	console.log("done? maybe. *crosses fingers* signing out, closing session!");
	exec("curl -k -c cookies.txt -b cookies.txt --progress-bar -o NUL -L https://bitbucket.org/account/signout/", signOutCallback);
}

function signOutCallback(error, stdout, stderr) {
	if (error) throw error;

	console.log("########## 100%");
}

function findLineWithText(searchText, allText) {
	var i;
	var lines = allText.split("\n");
	for (i=0; i<lines.length; i++) {
		if (lines[i].indexOf(searchText) >= 0)
			return lines[i];
	}
	return "";
}

startUpload();
