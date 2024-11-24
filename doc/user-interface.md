# Text-based Desktop Environment UI

## All modes (GUI/TUI)

<table>
<thead>
  <tr>
    <th rowspan="2"></th>
    <th colspan="3">Taskbar</th>
    <th colspan="4">App window</th>
    <th colspan="2">Desktop</th>
  </tr>
  <tr>
    <th>App group</th>
    <th>Running app</th>
    <th>User list</th>
    <th>Window controls</th>
    <th>Menu bar</th>
    <th>Interior</th>
    <th>Resize grips</th>
    <th>Navigation strings</th>
    <th>Free space</th>
  </tr>
</thead>
<tbody>
  <tr>
    <th>Ctrl-Alt ¹ (Alt+Shift+B on non-Windows platforms)</th>
    <td colspan="9">Toggle alternate keyboard mode</td>
  </tr>
  <tr>
    <th>Alt+Shift+N ¹</th>
    <td colspan="9">Run app</td>
  </tr>
  <tr>
    <th>F10 ¹</th>
    <td colspan="9">Disconnect all users and shutdown if there are no apps running</td>
  </tr>
  <tr>
    <th>Shift+F7 ¹</th>
    <td colspan="9">Leave current session</td>
  </tr>
  <tr>
    <th>Ctrl+PageUp/PageDown ¹</th>
    <td colspan="9">Switch focus between running apps</td>
  </tr>
  <tr>
    <th>LeftClick</th>
    <td>Run app</td>
    <td>Assign exclusive focus</td>
    <td></td>
    <td>Minimize<br>Maximize<br>Close</td>
    <td colspan="3">Assign exclusive focus</td>
    <td>Go to app</td>
    <td>Clear keyboard focus</td>
  </tr>
  <tr>
    <th>RightClick</th>
    <td>Set default app</td>
    <td></td>
    <td colspan="1"></td>
    <td colspan="2">Assign exclusive focus</td>
    <td colspan="1"></td>
    <td colspan="2">Center app window</td>
    <td></td>
  </tr>
  <tr>
    <th>MiddleClick</th>
    <td colspan="5"></td>
    <td colspan="1">Selection/clipboard paste</td>
    <td colspan="3"></td>
  </tr>
  <tr>
    <th>Left+RightClick</th>
    <td colspan="3"></td>
    <td colspan="5">Clear clipboard</td>
    <td></td>
  </tr>
  <tr>
    <th>Ctrl+LeftClick</th>
    <td colspan="1"></td>
    <td colspan="7">Assign/clear group keyboard focus</td>
    <td></td>
  </tr>
  <tr>
    <th>DoubleLeftClick</th>
    <td colspan="1"></td>
    <td colspan="1">Go to app window</td>
    <td colspan="1"></td>
    <td></td>
    <td colspan="2">Maximize<br>Restore</td>
    <td colspan="3"></td>
  </tr>
  <tr>
    <th>Alt+DoubleLeftClick</th>
    <td colspan="1"></td>
    <td colspan="1">Center app window</td>
    <td colspan="1"></td>
    <td></td>
    <td colspan="2"></td>
    <td colspan="3"></td>
  </tr>
  <tr>
    <th>Triple Left+RightClick</th>
    <td colspan="3">Toggle sysstat overlay</td>
    <td colspan="6"></td>
  </tr>
  <tr>
    <th>LeftDrag</th>
    <td colspan="3">Move desktop viewport</td>
    <td colspan="3">Move window or Select text</td>
    <td colspan="1">Resize window</td>
    <td colspan="1">Move window</td>
    <td>Move desktop viewport</td>
  </tr>
  <tr>
    <th>RightDrag</th>
    <td colspan="5"></td>
    <td>Panoramic content scrolling</td>
    <td colspan="2"></td>
    <td>Run default app</td>
  </tr>
  <tr>
    <th>MiddleDrag</th>
    <td colspan="9">Run default app</td>
  </tr>
  <tr>
    <th>Left+RightDrag</th>
    <td colspan="3"></td>
    <td colspan="4">Move window / Restore maximized</td>
    <td colspan="2">Move desktop viewport</td>
  </tr>
  <tr>
    <th>Ctrl+LeftDrag</th>
    <td colspan="3">Adjust folded width</td>
    <td colspan="3">Modify selection</td>
    <td colspan="1">Zoom window</td>
    <td colspan="2"></td>
  </tr>
  <tr>
    <th>Alt+LeftDrag</th>
    <td colspan="9">Switch boxed/linear selection mode</td>
  </tr>
  <tr>
    <th>Ctrl+RightDrag or Ctrl+MiddleDrag</th>
    <td colspan="9">Copy selected area to clipboard, OSC 52</td>
  </tr>
  <tr>
    <th>Wheel</th>
    <td colspan="7">Vertical scrolling</td>
    <td colspan="2"></td>
  </tr>
  <tr>
    <th>Shift+Wheel or Alt+Wheel</th>
    <td colspan="7">Horizontal scrolling</td>
    <td colspan="2"></td>
  </tr>
</tbody>
</table>

## GUI mode

<table>
<thead>
  <tr>
    <th></th>
    <th>GUI window</th>
    <th>Resizing grips</th>
    <th>Window 1px-height row (in fullscreen mode)²</th>
  </tr>
</thead>
<tbody>
  <tr>
    <th>Alt+Enter ¹</th>
    <td colspan="3">Toggle fullscreen mode</td>
  </tr>
  <tr>
    <th>Ctrl+CapsLock ¹</th>
    <td colspan="3">Toggle antialiasing mode</td>
  </tr>
  <tr>
    <th>CapsLock+UpArrow/DownArrow ¹</th>
    <td colspan="3">Scale cell size</td>
  </tr>
  <tr>
    <th>CapsLock+0 ¹</th>
    <td colspan="3">Reset cell size</td>
  </tr>
  <tr>
    <th>Ctrl+Wheel</th>
    <td colspan="3">Scale cell size (if unhandled)</td>
  </tr>
  <tr>
    <th>Ctrl+LeftClick</th>
    <td colspan="3">Assign/clear group keyboard focus</td>
  </tr>
  <tr>
    <th>DoubleLeftClick</th>
    <td colspan="3">Toggle fullscreen mode (if unhandled)</td>
  </tr>
  <tr>
    <th>AnyDrag<br>Left+RightDrag</th>
    <td colspan="1">Move GUI window (if unhandled)</td>
    <td colspan="1"></td>
    <td colspan="1"></td>
  </tr>
  <tr>
    <th>LeftDrag</th>
    <td colspan="1"></td>
    <td colspan="1">Resize GUI window</td>
    <td colspan="1"></td>
  </tr>
  <tr>
    <th>RightDrag</th>
    <td colspan="1"></td>
    <td colspan="1">Move GUI window</td>
    <td colspan="1"></td>
  </tr>
</tbody>
</table>

## Built-in Terminal

<table>
  <thead>
    <tr><th>Hotkey ¹</th>              <th>Default action</th></tr>
  </thead>
  <tbody>
    <tr><th>Alt+RightArrow</th>        <td>Highlight next match of selected text fragment. Clipboard content is used if no active selection.</td></tr>
    <tr><th>Alt+LeftArrow</th>         <td>Highlight previous match of selected text fragment. Clipboard content is used if no active selection.</td></tr>
    <tr><th>Shift+Ctrl+PageUp</th>     <td>Scroll one page up.</td></tr>
    <tr><th>Shift+Ctrl+PageDown</th>   <td>Scroll one page down.</td></tr>
    <tr><th>Shift+Alt+LeftArrow</th>   <td>Scroll one page to the left.</td></tr>
    <tr><th>Shift+Alt+RightArrow</th>  <td>Scroll one page to the right.</td></tr>
    <tr><th>Shift+Ctrl+UpArrow</th>    <td>Scroll one line up.</td></tr>
    <tr><th>Shift+Ctrl+DownArrow</th>  <td>Scroll one line down.</td></tr>
    <tr><th>Shift+Ctrl+LeftArrow</th>  <td>Scroll one cell to the left.</td></tr>
    <tr><th>Shift+Ctrl+RightArrow</th> <td>Scroll one cell to the right.</td></tr>
    <tr><th>Shift+Ctrl+Home</th>       <td>Don't repeat the Scroll to the scrollback top.</td></tr>
    <tr><th>Shift+Ctrl+Home</th>       <td>Scroll to the scrollback top.</td></tr>
    <tr><th>Shift+Ctrl+End</th>        <td>Don't repeat the Scroll to the scrollback bottom (reset viewport position).</td></tr>
    <tr><th>Shift+Ctrl+End</th>        <td>Scroll to the scrollback bottom (reset viewport position).</td></tr>
  </tbody>
</table>

---

¹ — Key bindings can be customized using settings.  
² — In fullscreen mode, the GUI window reserves a 1px high area at the top for forwarding mouse events.