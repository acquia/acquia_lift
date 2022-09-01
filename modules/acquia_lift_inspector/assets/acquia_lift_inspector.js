/**
 * @file
 * Contains the Lift Inspector Interface and behaviors.
 */

(function ($, Drupal) {

  'use strict';

  /**
   * Toolbar methods of Backbone objects.
   *
   * @namespace
   */
  Drupal.acquiaLiftInspector = {

    /**
     * Show the Acquia Lift Inspector Modal
     */
    showModal: function () {
      var $modal_element, modal_template;

      $('#lift-inspector').remove();

      //Get the Profile Manager URL from the config settings
      var accountURL = drupalSettings.acquia_lift_inspector.profile_manager_url;

      modal_template = _.template(
        '<div id="lift-inspector">' +
        '  <div class="lift-inspector-wrapper lift-inspector-wrapper-fullwidth">' +
        '    <h2>Account details</h2>' +
        '    <div id="account-details" class="container">' +
        '      <div class="item-wrapper">' +
        '        <label>' + Drupal.t('Customer') + '</label>' +
        '        <pre id="account-id" class="lift-value">' +
        '          <a href="' + accountURL + '#person:accountId=<%- account_id %>" target="_blank" rel="noopener noreferrer"><%- account_id %></a>' +
        '        </pre>' +
        '      </div>' +
        '      <div class="item-wrapper">' +
        '        <label>' + Drupal.t('Customer Site') + '</label>' +
        '        <pre id="site-id" class="lift-value"><%- site_id %></pre>' +
        '      </div>' +
        '      <div class="item-wrapper">' +
        '        <label>' + Drupal.t('Tracking ID') + '</label>' +
        '        <pre id="identity" class="lift-value">' +
        '        <% if (typeof lastDecisionRequest !== "undefined" && !_.isEmpty(lastDecisionRequest.identity)) { %>' +
        '            <a href="' + accountURL + '#person:accountId=<%- account_id %>;tracking=<%- lastDecisionRequest.identity %>" target="_blank" rel="noopener noreferrer"><%- lastDecisionRequest.identity %></a>' +
        '        <% } else { %>' + Drupal.t('No tracking id available.') + '<% } %>' +
        '        </pre>' +
        '      </div>' +
        '    </div>' +
        '  </div>' +
        '  <div class="lift-inspector-wrapper">' +
        '    <h2>' + Drupal.t('Current user segment(s)') + '</h2>' +
        '    <div id="user-segments" class="container ">' +
        '      <% if (typeof currentSegments !== "undefined" && !_.isEmpty(currentSegments)) { %>' +
        '        <% _.each(currentSegments, function (segment) {%>' +
        '          <div class="segment-item-wrapper item-wrapper">' +
        '           <div class="item-title">' +
        '             <h3 >' +
        '               <a href="' + accountURL + '#segment:accountId=<%- account_id %>;<%- encodeURIComponent(segment.name) %>,false" target="_blank" rel="noopener noreferrer"><%- segment.name %></a>' +
        '             </h3>' +
        '           </div>' +
        '           <div class="item-details">' +
        '             <div class="inline-wrap">' +
        '               <label class="inline-pre">' + Drupal.t('ID') + '</label>' +
        '               <pre><%- segment.id %></pre>' +
        '             </div>' +
        '             <div>' +
        '               <label>' + Drupal.t('Description') + '</label>' +
        '               <p class="lift-value"><%- segment.description %></p>' +
        '             </div>' +
        '           </div>' +
        '         </div>' +
        '        <% }); %>' +
        '      <% } else { %>' +
        '        <p>' + Drupal.t('No segment(s) available.') + '</p>' +
        '      <% } %>' +
        '    </div>' +
        '  </div>' +
        '  <div class="lift-inspector-wrapper">' +
        '    <h2>' + Drupal.t('Decision(s) on this page') + '</h2>' +
        '    <div id="decisions" class="container">' +
        '      <% if (typeof decisions !== "undefined" && !_.isEmpty(decisions)) { %>' +
        '        <% _.each(decisions, function (decision) {%>' +
        '          <div class="decision-item-wrapper item-wrapper">' +
        '            <div class="item-title">' +
        '              <h3>' + Drupal.t('Slot Name') + ': <%- decision.slot_name %>' +
        '              </h3>' +
        '            </div>' +
        '            <div class="item-details">' +
        '              <div>' +
        '                <label>' + Drupal.t('Slot View Mode') + '</label>' +
        '                <p class="lift-value"><%- decision.view_mode_id %></p>' +
        '              </div>' +
        '              <div>' +
        '                <label>' + Drupal.t('Rule Name') + '</label>' +
        '                <p class="lift-value"><%- decision.rule_name %></p>' +
        '              </div>' +
        '              <div>' +
        '                <label>' + Drupal.t('Content Displayed') + '</label>' +
        '                <p class="lift-value"><%- decision.content_name %></p>' +
        '              </div>' +
        '           </div>' +
        '          </div>' +
        '        <% }); %>' +
        '      <% } else { %>' +
        '        <p>' + Drupal.t('No decision(s) made.') + '</p>' +
        '      <% } %>' +
        '    </div>' +
        '  </div>' +
        '  <div class="lift-inspector-wrapper">' +
        '    <h2>' + Drupal.t('Recent captures') + '</h2>' +
        '    <div id="captures" class="container">' +
        '      <% if (typeof lastDecisionRequest !== "undefined" && !_.isEmpty(lastDecisionRequest.captures)) { %>' +
         '        <% _.each(lastDecisionRequest.captures, function (capture) {%>' +
        '          <div class="lastdecision-item-wrapper item-wrapper">' +
        '            <div class="item-title">' +
        '              <h3><%- capture.event_name %></h3>' +
        '            </div>' +
        '            <div class="item-details">' +
        '              <div>' +
        '                <label>' + Drupal.t('Source') + '</label>' +
        '                <p class="lift-value"><%- capture.event_source %></p>' +
        '              </div>' +
        '              <div class="inline-wrap">' +
        '                <label class="inline-pre">' + Drupal.t('URL') + '</label>' +
        '                <pre class="lift-value"><%- capture.url %></pre>' +
        '              </div>' +
        '            </div>' +
        '          </div>' +
        '        <% }); %>' +
        '      <% } else { %>' +
        '          <p>' + Drupal.t('No recent captures.') + '</p>' +
        '        </div>' +
        '      <% } %>' +
        '    </div>' +
        '  </div>' +
        '</div>'
      );

      if (typeof window.AcquiaLift === 'undefined') {
        $modal_element = '<h1 id="lift-inspector">' + Drupal.t('Lift is not currently loaded.') + '</h1>';
      } else {
        $modal_element = $(modal_template(window.AcquiaLift));
      }

      var dialogActiveClassName = 'dialog-active';
      var dialogContainerSelector = 'body';
      Drupal.dialog($modal_element, {
        title: Drupal.t('Lift Inspector'),
        show: { effect: 'fadeIn', duration: 200 },
        hide: { effect: 'fadeOut', duration: 200 },
        dialogClass: 'lift-inspector-dialog',
        minWidth: 900,
        create: function(event, ui) {$(dialogContainerSelector).addClass(dialogActiveClassName);},
        beforeClose: function(event, ui) {$(dialogContainerSelector).removeClass(dialogActiveClassName);}
      }).showModal();
    }
  };

  Drupal.behaviors.acquia_lift_inspector = {
    attach: function (context, settings) {
      $(document).keypress(function (e) {
        if (e.ctrlKey && e.which == '105' || e.which == '9') {
          Drupal.acquiaLiftInspector.showModal();
        }
      });
    }
  }

})(jQuery, Drupal);
