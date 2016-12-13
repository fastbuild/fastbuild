# This assumes that the 2 following variables are defined:
# - SONAR_HOST_URL => should point to the public URL of the SQ server (e.g. : https://sonarqube.com)
# - SONAR_TOKEN    => token of a user who has the "Execute Analysis" permission on the SQ server
# - GITHUB_TOKEN    => token for commenting pull requests in GitHub

Param(
	[parameter(Mandatory=$true)]
	[string]$sources,
	[string]$buildWrapperCommand	
)

# Special behavior for pull requests
if(Test-Path env:APPVEYOR_PULL_REQUEST_NUMBER) {
	Write-Output 'Pull Request Analysis'
	Write-Output $env:APPVEYOR_PULL_REQUEST_NUMBER
	.\SonarQubeAnalysis.ps1 -h $env:SONAR_HOST_URL -l $env:SONAR_TOKEN -s $sources -n $env:APPVEYOR_PROJECT_NAME -k $env:APPVEYOR_PROJECT_SLUG -v $env:APPVEYOR_BUILD_NUMBER -buildWrapperCommand $buildWrapperCommand -gitHubPullRequest $env:APPVEYOR_PULL_REQUEST_NUMBER -gitHubOauth $env:GITHUB_TOKEN -gitHubRepository $env:APPVEYOR_REPO_NAME
}
# Full analysis for dev branch only
ElseIf($env:APPVEYOR_REPO_BRANCH -eq 'dev') {
	Write-Output 'Full Analysis'
	.\SonarQubeAnalysis.ps1 -h $env:SONAR_HOST_URL -l $env:SONAR_TOKEN -s $sources -n $env:APPVEYOR_PROJECT_NAME -k $env:APPVEYOR_PROJECT_SLUG -v $env:APPVEYOR_BUILD_NUMBER -buildWrapperCommand $buildWrapperCommand
}
else {Write-Output 'Skipping Analysis'}
