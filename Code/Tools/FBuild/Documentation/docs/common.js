
function write( a ) { document.write(a); }

function generateHeader()
{
	generateHeaderinternal( "" );
}

function generateHeaderParent()
{
	generateHeaderinternal( "../" );
}


function generateHeaderinternal( path )
{
	write("<div style='height:15px;color:#D5EBFF;'></div>");
	write("<div style='margin:auto;width:980px;box-shadow: 0px 0px 1px 0px #888888;'>");
	write("<div>");
	write(" <div style='width:980px;height:80px;margin:auto;background-color:white;'>");
	write("  <a href='" + path + "home.html'><img src='" + path + "img/logo.png' style='position:relative;'/></a>");
	write("  <div style='display:inline-block;position:relative;left:410px;bottom:20px;'><a href='" + path + "contact.html' class='othernav'>Contact</a> &nbsp; | &nbsp; <a href='" + path + "license.html' class='othernav'>License</a></div>");
	write(" </div>");
	write("</div>");

	write("<div id='main'>");
	write("<div style='width:980px;position:relative;left:-10px;'>");
	write("<a href='" + path + "home.html' class='lnavbutton'>Home</a>");
	write("<div class='navbuttonbreak'><div class='navbuttonbreakinner'></div></div>");
	write("<a href='" + path + "features.html' class='navbutton'>Features</a>");
	write("<div class='navbuttonbreak'><div class='navbuttonbreakinner'></div></div>");
	write("<a href='" + path + "status.html' class='navbutton'>Status</a>");
	write("<div class='navbuttonbreak'><div class='navbuttonbreakinner'></div></div>");
	write("<a href='" + path + "documentation.html' class='navbutton'>Documentation</a>");
	
	write("<div class='navbuttongap'></div>");
	
	write("<a href='" + path + "download.html' class='rnavbutton'><b>Download</b></a>");
	write("</div>");

	write("<div style='width:auto;margin:0px;padding:10px;'>");
}

function generateFooter()
{
	write("</div>");
	write("<div class='footer'>&copy; 2012-2018 Franta Fulin</div>");
	write("</div>");
	write("</div>");
}

function caddr()
{
	write("<a href='");
	write("mai");
	write("lto:");
	write("fastbuild");
	write("@fa");
	write("stbui");
	write("ld.o");
	write("rg");
	write("'>");
	write("fastbuild");
	write("@fa");
	write("stbui");
	write("ld.o");
	write("rg");
	write("</a>");
}