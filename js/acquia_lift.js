var _tcaq = _tcaq || [];
var _tcwq = _tcwq || [];

(function ($, Drupal, drupalSettings) {

  "use strict";

  var credential = drupalSettings.acquia_lift.credential;
  _tcaq.push(['setAccount', credential.account_name]);
  (function() {
    function async_load()
    {
       var s = document.createElement('script');
       s.type = 'text/javascript';
       s.async = true;
       s.src = ('https:' == document.location.protocol ? 'https' : 'http') + '://' + credential.js_path;
       var x = document.getElementsByTagName('script')[0];
       x.parentNode.insertBefore(s, x);
    }
    if (window.attachEvent)
       window.attachEvent('onload', async_load);
    else
       window.addEventListener('load', async_load, false);
  })();

})(jQuery, Drupal, drupalSettings);
