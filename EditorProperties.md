![http://foo-wsh-panel-mod.googlecode.com/files/Preferences.png](http://foo-wsh-panel-mod.googlecode.com/files/Preferences.png)



# Editor Properties Customization #

Thanks to the powerful [Scintilla](http://www.scintilla.org/) editor component, these styles are now provided for customization:

  * Common Styles
    * style.default
    * style.comment
    * style.keyword
    * style.indentifier
    * style.string
    * style.number
    * style.operator
    * style.linenumber
    * style.bracelight
    * style.bracebad
  * Common Variables
    * style.selection.fore
    * style.selection.back
    * style.selection.alpha
    * style.caret.fore
    * style.caret.width
    * style.caret.line.back
    * style.caret.line.back.alpha
  * Extra Variables
    * api.vbscript
    * api.jscript
  * Embeded Properties
    * $(dir.base)
    * $(dir.component)
    * $(dir.profile)


## Common Styles ##

This message is quoted from [SciTE Doc](http://www.scintilla.org/SciTEDoc.html)

> The value of each setting is a set of ',' separated fields, some of which have a subvalue after a ':'.The fields are **font**, **size**, **fore**, **back**, **italics**, **notitalics**, **bold**, **notbold**, ~~eolfilled~~, ~~noteolfilled~~, **underlined**, **notunderlined**, and **case**. The font field has a subvalue which is the name of the font, the fore and back have colour subvalues, the size field has a numeric size subvalue, the case field has a subvalue of '**m**', '**u**', or '**l**' for mixed, upper or lower case, and the bold, italics and eolfilled fields have no subvalue. The value "**fore:#FF0000,font:Courier,size:14**" represents **14 point, red Courier text**.

## Common Variables ##

<table cellpadding='1' border='1' cellspacing='0'>
<thead><tr><th>Name</th><th>Description</th></tr></thead>
<tbody>
<tr>
<td>style.selection.fore<br />style.selection.back<br />style.selection.alpha</td>
<td>Sets the colours used for displaying selected text. If one of these is not set then that attribute is not changed for the selection. The default is to show the selection by changing the background to light grey and leaving the foreground the same as when it was not selected. The translucency of the selection is set with selection.alpha with an alpha of 256 turning translucency off.</td>
</tr>
<tr>
<td>style.caret.fore</td>
<td>Sets the colour used for the caret.</td>
</tr>
<tr>
<td>style.caret.width</td>
<td>Sets the width of the caret in pixels. Only values of 1, 2, or 3 work.</td>
</tr>
<tr>
<td>style.caret.line.back<br />style.caret.line.back.alpha</td>
<td>Sets the background colour and translucency used for line containing the caret. Translucency ranges from 0 for completely transparent to 255 for opaque with 256 being opaque and not using translucent drawing code which may be slower.</td>
</tr>
</tbody>
</table>

## Extra Variables ##

<table cellpadding='1' border='1' cellspacing='0'>
<thead><tr><th>Name</th><th>Description</th></tr></thead>
<tbody>
<tr>
<td>api.vbscript<br />api.jscript</td>
<td>API files are used for auto completion and calltips, SciTE compatible.</td>
</tr>
</tbody>
</table>

## Embeded Properties ##

<table cellpadding='1' border='1' cellspacing='0'>
<thead><tr><th>Name</th><th>Description</th></tr></thead>
<tbody>
<tr>
<td>$(dir.base)</td>
<td>foobar2000 path (trailing '\' is included), <b>eg</b>: C:\foobar2000\</td>
</tr>
<tr>
<td>$(dir.component)</td>
<td>foobar2000 component path (trailing '\' is included), <b>eg</b>: C:\foobar2000\components\</td>
</tr>
<tr>
<td>$(dir.profile)</td>
<td>foobar2000 profile path (trailing '\' is included, since WSH Panel Mod 1.3.7)</td>
</tr>
</tbody>
</table>
<br />

# Scripting Host Setting #

## Timeout ##
> Prevent scripting host from hanging.
> Set to 0 to disable this feature.

## Debug mode ##
> Enable/Disable JIT Script Debugging. Useful if you have one of the script debuggers installed:
    1. Microsoft Visual Studio 2008/2010
    1. Microsoft Script Debugger (Disrecommended since it's out of date)
> The script debugger will be started when script error occurred, it's also possible to start a script debugger by holding **Left-Shift, Left-Ctrl, Left-Win and Left-Alt**, then right-click on one of your panels of WSH Panel Mod.


## Safe mode ##
> When enabled, the scripting host will refuse to create some potential harmful [ActiveX](http://en.wikipedia.org/wiki/ActiveX) objects, such as [FileSytemObject](http://msdn.microsoft.com/en-us/library/aa242706%28VS.60%29.aspx).
> This option is enabled by default since 1.3.0 Beta 4, due to some potential security risks.