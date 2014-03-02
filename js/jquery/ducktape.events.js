// Backbone has a dependency on jQuery 1.7+ because it refers to the
// jQuery.fn.on and jQuery.fn.off methods. We polyfill them here, but prefer
// the native implementation if it exists.
if (!jQuery.fn.on && !jQuery.fn.off) {
  jQuery.fn.on = jQuery.fn.bind;
  jQuery.fn.off = jQuery.fn.unbind;
}
