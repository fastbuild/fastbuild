Param(
	[parameter(Mandatory=$true)]
	[alias("h")]
	[string]$hostUrl,
	[parameter(Mandatory=$true)]
	[alias("l")]
	[string]$login,
	[parameter(Mandatory=$true)]
	[alias("n")]
	[string]$projectName,
	[parameter(Mandatory=$true)]
	[alias("k")]
	[string]$projectKey,
	[parameter(Mandatory=$true)]
	[alias("b")]
	[string]$projectVersion,
	[parameter(Mandatory=$true)]
	[alias("s")]
	[string]$sources,
	[string]$buildWrapperCommand 
)

Add-Type -assembly system.io.compression.filesystem

# Download and unzip sonnar scanner
if(![System.IO.Directory]::Exists($PSScriptRoot + '\SonarScanner'))
{
	(new-object net.webclient).DownloadFile('http://repo1.maven.org/maven2/org/sonarsource/scanner/cli/sonar-scanner-cli/2.6/sonar-scanner-cli-2.6.zip', $PSScriptRoot +  '\SonarScanner.zip')
	[io.compression.zipfile]::ExtractToDirectory($PSScriptRoot + '\SonarScanner.zip', $PSScriptRoot + '\SonarScanner')
}

$scannerCmdLine = ".\SonarScanner\sonar-scanner-2.6\bin\sonar-scanner.bat -D sonar.host.url='$hostUrl' -D sonar.login='$login' -D sonar.projectKey='$projectKey' -D sonar.projectName='$projectName' -D sonar.projectVersion='$projectVersion' -D sonar.sources='$sources'"

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

Write-Output $scannerCmdLine

Invoke-Expression $scannerCmdLine
