function OpenURL(url, target) {
	if (arguments.length == 2 && target.length > 0) {
		window.open(url,target);
	} else {
		document.location.href = url;
	}
}
function SwapStyle(o, classname) {
	o.className = classname;
}

function ClearInput(o,n) {
	if (o.value == n)
		o.value = "";
}
function RestoreInput(o,n) {
	if (o.value == "")
		o.value = n;
}

function IsUndefined(v) {
	return typeof v == 'undefined';
}

function GetElem(e) {
	if ( typeof e  == "string" && !IsUndefined(document.getElementById) ) {
		e = document.getElementById(e);
	}
	return e;
}

function flipAnswer(id) {
	var answer = document.getElementById("answer"+id);
	var img = document.getElementById("img"+id);
	if (answer && img) {
		if (answer.style.display == 'none') {
			img.src = "/images/contr.gif";
			answer.style.display = 'block';
		} else {
			img.src = "/images/exp.gif";
			answer.style.display = 'none';
		}
	}
}

// Print
function printPage() 
{
	var da = (document.all) ? 1 : 0;
	var pr = (window.print) ? 1 : 0;
	
	if(!pr)
		return;
	
	var printArea = document.getElementById("Main");
	
	if(printArea == null && da) 
		printArea = document.all.mainareadiv;
	
	if(printArea) 
	{
		var sStart = "<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles/structure.css\"><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles/editor.css\"><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles/units.css\"><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles/print.css\"></head><body onload=\"javascript:window.print();\">";
		var sHeader = "<div style='width:100%;overflow:hidden;height:72px;margin-bottom:10px;'><img src='/images/bg_header.gif'></div>";
		var sStop = "</body></html>";

		var w = window.open('about:blank','printWin','width=650,height=440,scrollbars=yes');
		wdoc = w.document;
		wdoc.open();
		wdoc.writeln( sStart ) ;
		wdoc.writeln( sHeader );
		wdoc.writeln( printArea.innerHTML );
		wdoc.writeln( sStop );

		w.document.close();
		if (window.print) {
			w.print();
			w.close(); 
		}
	}
}

// Footer
function getHeight() {
	if ( typeof window.innerHeight  == 'number' ) return window.innerHeight;
	else if ( document.documentElement && document.documentElement.clientWidth ) return document.documentElement.clientHeight;
	else return document.body.clientHeight;
}

function resize() {
	var m = document.getElementById("middle");
	var h = getHeight();
	if (m&&h) {
		m.style.height=h-195+"px";
		m.style.minHeight=h-192+"px";
	}
}
