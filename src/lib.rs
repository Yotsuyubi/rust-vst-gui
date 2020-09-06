#[cfg(windows)]
#[macro_use]
extern crate memoffset;
#[cfg(windows)]
#[macro_use]
extern crate winapi;
extern crate vst;

#[cfg(target_os = "macos")]
extern crate cocoa;

#[cfg(target_os = "macos")]
extern crate webview_sys;

use std::error::Error;
use std::os::raw::c_void;

#[cfg(windows)]
mod win32;

#[cfg(target_os = "macos")]
mod macos;

mod lib {
    use std::error::Error;
    use std::os::raw::c_void;

    pub type JavascriptCallback = Box<dyn Fn(String) -> String>;

    pub trait PluginGui {
        fn size(&self) -> (i32, i32);
        fn position(&self) -> (i32, i32);
        fn close(&mut self);
        fn open(&mut self, parent_handle: *mut c_void) -> bool;
        fn is_open(&mut self) -> bool;
        fn execute(&self, javascript_code: &str) -> Result<(), Box<dyn Error>>;
    }
}

pub struct PluginGui {
    gui: Box<dyn lib::PluginGui>,
}

impl PluginGui {
    // Calls the Javascript 'eval' function with the specified argument.
    // This method always returns an error when the plugin window is closed.
    pub fn execute(&self, javascript_code: &str) -> Result<(), Box<dyn Error>> {
        self.gui.execute(javascript_code)
    }
}

impl vst::editor::Editor for PluginGui {
    fn size(&self) -> (i32, i32) {
        self.gui.size()
    }

    fn position(&self) -> (i32, i32) {
        self.gui.position()
    }

    fn close(&mut self) {
        // close window before close gui.
        self.execute("window.open('about:blank','_self').close()");
        self.gui.close()
    }

    fn open(&mut self, parent_handle: *mut c_void) -> bool {
        self.gui.open(parent_handle)
    }

    fn is_open(&mut self) -> bool {
        self.gui.is_open()
    }
}

pub use lib::JavascriptCallback;

pub fn new_plugin_gui(
    html_document: String, js_callback: JavascriptCallback, window_size: Option<(i32, i32)>) -> PluginGui
{
    #[cfg(windows)]
    {
        PluginGui {gui: win32::new_plugin_gui(html_document, js_callback, window_size) }
    }
    #[cfg(target_os = "macos")]
    {
        PluginGui {gui: macos::new_plugin_gui(html_document, js_callback, window_size) }
    }
}
