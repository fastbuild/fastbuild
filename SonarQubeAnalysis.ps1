
Param(
	# Analysis parameters (see http://docs.sonarqube.org/display/SONAR/Analysis+Parameters)
	[parameter(Mandatory=$true,HelpMessage="Server URL ")]
	[alias("h")]
	[string]$hostUrl,
	[parameter(Mandatory=$true,HelpMessage="The authentication token of a SonarQube user with Execute Analysis permission.")]
	[alias("l")]
	[string]$login,
	[parameter(Mandatory=$true,HelpMessage="Name of the project that will be displayed on the web interface.\nSet through <name> when using Maven.")]
	[alias("n")]
	[string]$projectName, 
	[parameter(Mandatory=$true,HelpMessage="The project key that is unique for each project.\nAllowed characters are: letters, numbers, '-', '_', '.' and ':', with at least one non-digit.\nWhen using Maven, it is automatically set to <groupId>:<artifactId>.")]
	[alias("k")]
	[string]$projectKey,
	[parameter(Mandatory=$true,HelpMessage="The project version.\nSet through <version> when using Maven.")]
	[alias("v")]
	[string]$projectVersion,
	[parameter(Mandatory=$true, HelpMessage="Comma-separated paths to directories containing source files.\nCompatible with Maven. If not set, the source code is retrieved from the default Maven source code location. ")]
	[alias("s")]
	[string]$sources,
	#Option build wrapper command (for C/C++/Objective-C builds)
	[string]$buildWrapperCommand,
	# Pull request specific arguments (see http://docs.sonarqube.org/display/PLUG/GitHub+Plugin)
	[parameter(HelpMessage="Pull request number")]
	[int]$gitHubPullRequest,
	[parameter(HelpMessage="Personal access token generated in GitHub for the technical user ")]
	[string]$gitHubOauth,	
	[parameter(HelpMessage="Identification of the repository. Format is: <organisation/repo>. Exemple: SonarSource/sonarqube	")]
	[string]$gitHubRepository
)

Add-Type -assembly system.io.compression.filesystem

# Download and unzip sonnar scanner
if(![System.IO.Directory]::Exists($PSScriptRoot + '\SonarScanner'))
{
	(new-object net.webclient).DownloadFile('http://repo1.maven.org/maven2/org/sonarsource/scanner/cli/sonar-scanner-cli/2.8/sonar-scanner-cli-2.8.zip', $PSScriptRoot +  '\SonarScanner.zip')
	[io.compression.zipfile]::ExtractToDirectory($PSScriptRoot + '\SonarScanner.zip', $PSScriptRoot + '\SonarScanner')
}

$scannerCmdLine = ".\SonarScanner\sonar-scanner-2.8\bin\sonar-scanner.bat -D sonar.host.url='$hostUrl' -D sonar.login='$login' -D sonar.projectKey='$projectKey' -D sonar.projectName='$projectName' -D sonar.projectVersion='$projectVersion' -D sonar.sources='$sources'"

#Download build wrapper (if needed)
if($buildWrapperCommand)
{
	if(![System.IO.Directory]::Exists($PSScriptRoot +  '\BuildWrapper'))
	{
		[System.Net.ServicePointManager]::SecurityProtocol = @('Tls12','Tls11','Tls','Ssl3')
		(new-object net.webclient).DownloadFile('http://sonarqube.com/static/cpp/build-wrapper-win-x86.zip', $PSScriptRoot + '\BuildWrapper.zip')
		[io.compression.zipfile]::ExtractToDirectory($PSScriptRoot + '\BuildWrapper.zip', $PSScriptRoot + '\BuildWrapper')
	}
	
	# Compile with BuildWrapper
	
    $builderCmdLine = ".\BuildWrapper\build-wrapper-win-x86\build-wrapper-win-x86-64.exe --out-dir 'Build' $buildWrapperCommand"
	Write-Output $builderCmdLine
	Invoke-Expression $builderCmdLine
	
	$scannerCmdLine += ' -D sonar.cfamily.build-wrapper-output=Build'
}

# Pull request ?
if($gitHubPullRequest)
{
	$scannerCmdLine += " -D sonar.analysis.mode=preview -D sonar.github.oauth='$gitHubOauth' -D sonar.github.repository='$gitHubRepository' -D sonar.github.pullRequest='$gitHubPullRequest'"
}

Write-Output $scannerCmdLine

Invoke-Expression $scannerCmdLine
