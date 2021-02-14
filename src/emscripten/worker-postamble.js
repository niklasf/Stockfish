//
// Patch `onmessage` to support custom message
//
const oldOnmessage = self.onmessage;

self.onmessage = (e) => {
  if (e.data.cmd === "custom") {
    if (typeof Module["onCustomMessage"] === "function") {
      Module["onCustomMessage"](e.data.userData);
    }
  } else {
    oldOnmessage(e);
  }
};
