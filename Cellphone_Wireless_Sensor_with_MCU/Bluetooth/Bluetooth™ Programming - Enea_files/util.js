/// uses X, cross-browser library (cross-browser.com) (GNU LGPL)

function hasClass(el,c) 
{
	var re = new RegExp('\\b'+c+'\\b', 'i');
    return re.test(xGetElementById(el).className);
}

function removeClassName(el, c)
{
	el = xGetElementById(el);
	if ( c.length > 0 && el.className.length > 0 )
	{
		var re = new RegExp('\\b'+c+'\\b', 'ig');
		el.className = el.className.replace(re, "").trim();
	}
}

function addClassName(el, c) 
{
	el = xGetElementById(el);
	if ( !hasClass(el) ) { el.className = (el.className + " " + c).trim();}
}

function getAttribute(el, name)
{
    var attr = el.getAttribute(name);
    return attr ? attr : el[name];
}

function unselectable(el)
{
	el = xGetElementById(el);
	if (el)
	{
		el.style.MozUserSelect = "none";
		el.style.KhtmlUserSelect = "none";
		el.style.userSelect = "none";
		el.unselectable = "on"; 
	}
}

function setOverflow(el, y, x)
{
	el = xGetElementById(el);
	if (el)
	{
		if (y&&x)
		{
			el.style.overflow = "auto";
		}
		else if (y)
		{
			if (xGecko) el.style.overflow = "-moz-scrollbars-vertical";
			el.style.overflowY = "auto";
			el.style.overflowX = "hidden";
		}
		else if (x)
		{
			if (xGecko) el.style.overflow = "-moz-scrollbars-horizontal";
			el.style.overflowX = "auto";
			el.style.overflowY = "hidden";
		}
		else 
		{
			el.style.overflow = "hidden";
		}
	}
}

function setOpacity(el,val) {
    if (el.filters) { try { el.filters['alpha'].opacity = val*100; } catch(e){} } 
    else if (el.style.opacity) { el.style.opacity = val; }
}

function dispatchEvent(el, name, bubbles, cancelable) 
{
	bubbles = bubbles || true;
	cancelable = cancelable || true;
	
	if ( document.createEvent && el.dispatchEvent ) 
	{
		var e = document.createEvent("HTMLEvents");
		e.initEvent(name, bubbles, cancelable); 
		el.dispatchEvent(e); 
	} 
	else if (el.fireEvent)  
	{ 
		var e = document.createEventObject();
		e.cancelBubble = bubbles;
		el.fireEvent("on" + name, e); 
	}
	else if ( eval("el.on" + name) ) 
	{ 
		eval("el.on" + name + "()"); 
	}
}

/**************************************
/* ARRAY EXTENSIONS
/**************************************/
if (!Array.prototype.push) Array.prototype.push = function() 
{
    for (var i=0; i<arguments.length; i++) this[this.length] = arguments[i];
    return this.length;
}

if (!Array.prototype.find) Array.prototype.find = function(value, start) 
{
    start = start || 0;
    for (var i=start; i<this.length; i++)
        if (this[i]==value)
            return i;
    return -1;
}

if (!Array.prototype.has) Array.prototype.has = function(value) 
{
    return this.find(value)!==-1;
}


/**************************************
/* STRING EXTENSIONS
/**************************************/
String.prototype.trim = function() { 
	return this.replace( /^\s+|\s+$/, "" ); 
}


/**************************************
/* FUNCTIONAL
/**************************************/
function map(arr, func) {
    var result = [];
    func = func || function(v) {return v};
    for (var i=0; i < arr.length; i++) result.push(func(arr[i], i, arr));
    return result;
}

function filter(arr, func) {
    var result = [];
    func = func || function(v) {return v};
    map(arr, function(v) { if (func(v)) result.push(v) } );
    return result;
}

/**************************************
/* POP-UP FUNCTIONS
/**************************************/
var _POPUP_FEATURES = "";

// pops up a window containing url optionally named target, optionally having features
function raw_popup(url, target, features) {
   	// Set parent window name - for easy reference from popup
   	if ( window.name.length <= 0 )
	   	window.name = "WindowOpener";

    if ( !xDef(features) ) {
		features = _POPUP_FEATURES;
	}
    if ( !xDef(target) ) {
		target   = '_blank';
	}
    var popupWindow = window.open(url, target, features);
    popupWindow.focus();
    return popupWindow;
}

function link_popup(src, features) {
    return raw_popup(src.getAttribute('href'), src.getAttribute('target') || '_blank', features);
}

function event_popup(e) {
    link_popup(e.target);
    e.preventDefault();
}

function event_popup_features(features) 
{
    return function(e) { link_popup(e.target || event.srcElement, features); xPreventDefault(e); };
}

function linksAsPopup(target, features) 
{
	var callback = function(el) { xAddEventListener(el, "click", event_popup_features(features)); };
	xGetElementsByAttribute("a", "target", target, callback);
}

/**************************************
/* POSITION FUNCTIONS
/**************************************/

function absoluteLeft(el)
{
	return el ? xOffsetLeft(el) + absoluteLeft(el.offsetParent) : 0;
}

function absoluteTop(el)
{
	return el ? xOffsetTop(el) + absoluteTop(el.offsetParent) : 0;
}