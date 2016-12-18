/**
 * @file acquia_lift_inspector.js
 */
(function ($, Drupal) {
  'use strict';

  Drupal.behaviors.acquia_lift = {
    attach: function (context, settings) {

      function show_inspector_modal() {
        var $modal_element, modal_template;

        $('#lift-inspector').remove();

        //Get the Profile Manager URL from the config settings
        var accountSubDomain = drupalSettings.acquia_lift.acquia_lift_inspector.profile_manager_url;

        if (accountSubDomain) {
          var accountURL = 'https://' + accountSubDomain + '.lift.acquia.com';
        } else {
          var accountURL = 'http://lift.acquia.com';
        };

        modal_template = _.template(
          '<div id="lift-inspector">' +
          '  <div class="lift-inspector-wrapper lift-inspector-wrapper-fullwidth">' +
          '    <h4>Account details</h4>' +
          '    <div class="account-details container">' +
          '     <div class="item-wrapper"><label>' + Drupal.t('Customer') + '</label><pre class="liftValue"><a href="' + accountURL + '#person:accountId=<%- account_id %>" target="_blank" rel="noopener noreferrer"><%- account_id %></a></pre></div>' +
          '     <div class="item-wrapper"><label>' + Drupal.t('Customer Site') + '</label><pre class="liftValue"><%- site_id %></a></pre></div>' +
          '     <div class="item-wrapper"><label>' + Drupal.t('Tracking ID') + '</label><pre class="liftValue"><a href="' + accountURL + '#person:accountId=<%- account_id %>;tracking=<%- lastDecisionRequest.identity %>" target="_blank" rel="noopener noreferrer"><%- lastDecisionRequest.identity %></a></pre></div>' +
          '    </div>' +
          '  </div>' +
          '  <div class="lift-inspector-wrapper">' +
          '    <h4>' + Drupal.t('Current user segment(s)') + '</h4>' +
          '    <div class="user-segments container ">' +
          '      <% if (!_.isEmpty(currentSegments)) { %>' +
          '        <% _.each(currentSegments, function (segment) {%>' +
          '          <div class="segment-item-wrapper item-wrapper">' +
          '           <div class="item-title"><h3 ><a href="' + accountURL + '#segment:accountId=<%- account_id %>;<%- encodeURIComponent(segment.name) %>,false" target="_blank" rel="noopener noreferrer"><%- segment.name %></a></h3></div>' +
          '           <div class="item-details">' +
          '             <div class="inline-wrap"><label class="inline-pre">' + Drupal.t('ID') + '</label><pre><%- segment.id %></pre></div>' +
          '             <div><label>' + Drupal.t('Description') + '</label><p class="liftValue"><%- segment.description %></p></div>' +
          '           </div>' +
          '         </div>' +
          '        <% }); %>' +
          '      <% } else { %>' +
          '        <p>' + Drupal.t('No segment(s) available.') + '</p>' +
          '      <% } %>' +
          '    </div>' +
          '  </div>' +
          '  <div class="lift-inspector-wrapper">' +
          '    <h4>' + Drupal.t('Decisions on this page') + '</h4>' +
          '    <div class="decisions container">' +
          '      <% if (!_.isEmpty(decisions)) { %>' +
          '        <% _.each(decisions, function (decision) {%>' +
          '          <div class="decision-item-wrapper item-wrapper">' +
          '            <div class="item-title"><h3>' + Drupal.t('Slot Name') + ': <%- decision.slot_name %></p></div>' +
          '           <div class="item-details">' +
          '             <div><label>' + Drupal.t('Slot View Mode') + '</label><p class="liftValue"><%- decision.view_mode_id %></p></div>' +
          '             <div><label>' + Drupal.t('Rule Name') + '</label><p class="liftValue"><%- decision.rule_name %></p></div>' +
          '             <div><label>' + Drupal.t('Content Displayed') + '</label><p class="liftValue"><%- decision.content_name %></p></div>' +
          '           </div>' +
          '          </div>' +
          '        <% }); %>' +
          '      <% } else { %>' +
          '        <div class="item-wrapper"><p>' + Drupal.t('No decisions made.') + '</p></div>' +
          '      <% } %>' +
          '    </div>' +
          '  </div>' +
          '  <div class="lift-inspector-wrapper">' +
          '    <h4>' + Drupal.t('Recent captures') + '</h4>' +
          '    <div class="captures container">' +
          '      <% if (!_.isEmpty(lastDecisionRequest.captures)) { %>' +
          '        <% _.each(lastDecisionRequest.captures, function (capture) {%>' +
          '          <div class="lastdecision-item-wrapper item-wrapper">' +
          '          <div class="item-title"><h3><%- capture.event_name %></h3></div>' +
          '           <div class="item-details">' +
          '          <div><label>' + Drupal.t('Source') + '</label><p class="liftValue"><%- capture.event_source %></p></div>' +
          '          <div class="inline-wrap"><label class="inline-pre">' + Drupal.t('URL') + '</label><pre class="liftValue"><%- capture.url %></pre></div>' +
          '          <div> <label>' + Drupal.t('Date') + '</label><p class="liftValue"><%- capture.event_date %></p></div>' +
          '           </div>' +
          '          </div>' +
          '        <% }); %>' +
          '      <% } else { %>' +
          '        <div class="item-details"><p class="liftValue">' + Drupal.t('No recent captures.') + '</p></div>' +
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

      // $(document).ready(function(){
      //   show_inspector_modal();
      // })


      $(document).keypress(function (e) {
        if (e.ctrlKey && e.which == '105' || e.which == '9') {
          show_inspector_modal();
        }
      });
    }
  }

}(jQuery, Drupal));
