<?php

/**
 * @file acquia_lift.ui.inc
 * Provides functions needed for the front-end UI.
 */

/**
 * Menu callback; Provide the top-level access point.
 */
function acquia_lift_root_page() {
  drupal_goto('admin/structure/personalize');
}

/**
 * Sends the Acquia Lift control menus.
 */
function acquia_lift_controls_assets_callback() {
  // The Acquia Lift module is responsible for assembling menu items into a
  // single menu in the navbar.
  $menu = menu_tree_all_data('acquia-lift-controls');

  $response = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-controls'),
    ),
    'personalization' => menu_tree_output($menu),
  );
  return $response;
}

/**
 * Attaches the front-end controls to the page.
 *
 * @param $page
 *   The render array of the page.
 */
function acquia_lift_build_page(&$page) {
  // Attach the editor app toggle code on all non-admin pages.
  // A special case is made for the block demo page which is not properly
  // marked as an admin page by path_is_admin().
  if (!path_is_admin(current_path()) && preg_match('/^admin\/structure\/block\/demo\//', current_path()) == 0) {

    // If the user is personalizing content in context, provide a DSM with actions
    // to personalize or cancel out of this 'mode'.
    if (acquia_lift_nav_message_is_set()) {
      drupal_set_message(acquia_lift_edit_mode_personalize_in_context(), 'acquia-lift-personalize');
    }
  }

  // Attach client-side controls for managing personalized content.
  $page['page_top']['acquia_lift']['#access'] = user_access('manage personalized content');
  $page['page_top']['acquia_lift']['#attached']['library'][] = array('acquia_lift', 'acquia_lift.personalize');
  // The controls JavaScript file is only necessary if the Navbar module is
  // not installed.
  if (!module_exists('navbar')) {
    // Don't add the dialog behavior to admin pages either.
    if (!path_is_admin(current_path()) && preg_match('/^admin\/structure\/block\/demo\//', current_path()) == 0) {
      $page['page_top']['acquia_lift']['#attached']['library'][] = array('acquia_lift', 'acquia_lift.controls');
    }
  }
}

/**
 * Attaches the jQuery "chosen" behavior to the the passed in element.
 *
 * @param array $element
 *   An array representing a multi-select form element.
 * @param array $classes
 *   An array of classes to be included on this element.
 */
function acquia_lift_chosenify_element(&$element, $classes = array()) {
  $chosen_path = libraries_get_path('chosen');
  $classes[] = 'acquia-lift-chosen-select';

  $options = array(
    'scope' => 'footer',
    'defer' => TRUE,
  );
  $element['#attributes'] = array(
    'class' => $classes,
    'data-placeholder' => t('Choose a context...'),
  );
  $element['#attached'] = array(
    'js' => array(
      $chosen_path . '/chosen.jquery.min.js' => array('group' => 'JS_LIBRARY'),
      drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.admin.js' => $options,
    ),
    'css' => array(
      $chosen_path . '/chosen.css' => array(),
    )
  );
}

/**
 * =======================================================================
 *  F U N C T I O N S  B A S E D  O N  V I S I T O R  A C T I O N S  U I
 * =======================================================================
 */


/**
 * Builds controls for an in-context personalization mode.
 *
 * Based on visitor_actions_ui_edit_mode_action_in_context().
 *
 * @return String
 *   The rendered string for display.
 */
function acquia_lift_edit_mode_personalize_in_context() {
  $build = array(
    'message' => array(
      '#markup' => '<p>' . t('To personalize content in context, navigate to the page where the content you wish to personalize is, then press the %link link.', array('%link' => 'Personalize')) . '</p>',
    ),
    'actions' => array(
      '#theme' => 'acquia_lift_edit_mode_personalize_in_context_links',
      '#attributes' => array(
        'class' => array('visitor-actions-ui-actions'),
      ),
      'launch' => array(
        '#type' => 'link',
        '#title' => t('Personalize'),
        '#href' => drupal_get_normal_path('admin/structure/personalize'),
        '#options' => array(
          'attributes' => array(
            'title' => t('Highlight content that can be personalized'),
            'role' => 'button',
            'aria-pressed' => 'false',
            'class' => array('acquia-lift-ignore', 'acquia-lift-personalize-trigger'),
            'data-acquia-lift-personalize-mode' => 'content-variation',
          ),
        ),
      ),
      'cancel' => array(
        '#type' => 'link',
        '#title' => t('Cancel'),
        '#href' => drupal_get_normal_path('acquia_lift/personalize_in_context/stop'),
        '#options' => array(
          'attributes' => array(
            'title' => t('Home page'),
            'class' => array('acquia-lift-ignore'),
          ),
        ),
      ),
    ),
  );

  // Allow other modules to modify the message markup.
  drupal_alter('acquia_lift_edit_mode_actions', $build);
  return drupal_render($build);
}

/**
 * Toggles acquia_lift edit mode.
 */
function acquia_lift_edit_mode_toggle() {
  if (isset($_SESSION['acquia_lift_edit_mode'])) {
    unset($_SESSION['acquia_lift_edit_mode']);
  }
  else {
    $_SESSION['acquia_lift_edit_mode'] = 1;
  }
}

/**
 * Disables edit mode and returns to the stored destination.
 */
function acquia_lift_edit_mode_disable() {
  unset($_SESSION['acquia_lift_edit_mode']);
  $goto = '';
  $options = array();
  if (isset($_SESSION['acquia_lift_personalize_edit_mode_return'])) {
    $goto = $_SESSION['acquia_lift_personalize_edit_mode_return'];
    if (strpos($goto, '#overlay=') === 0) {
      // If the original path was in the overlay, make sure we go back
      // to the overlay.
      $path = substr($goto, strlen('#overlay='));
      $goto = current_path();
      $options = array('fragment' => 'overlay=' . $path);
    }
    unset($_SESSION['acquia_lift_personalize_edit_mode_return']);
    // The above session variable trumps any destination that has been
    // set so we need to make sure drupal_goto() behaves correctly.
    if (isset($_GET['destination'])) {
      unset($_GET['destination']);
    }
  }
  elseif (isset($_GET['destination'])) {
    $goto = $_GET['destination'];
    unset($_GET['destination']);
  }
  if (acquia_lift_nav_message_is_set()) {
    acquia_lift_unset_nav_message();
  }
  drupal_goto($goto, $options);
}

/**
 * Page callback to start personalization edit mode.
 */
function personalize_in_context_start() {
  if (isset($_GET['destination'])) {
    $_SESSION['acquia_lift_personalize_edit_mode_return'] = $_GET['destination'];
    unset($_GET['destination']);
  }
  acquia_lift_set_nav_message();
  drupal_goto();
}

/**
 * Sets the instructional message for users to navigate to where they
 * wish to add an action.
 */
function acquia_lift_set_nav_message() {
  $_SESSION['acquia_lift_personalize_set_message'] = 1;
}

/**
 * Checks whether the instructional navigation message is set.
 *
 * @return bool
 *   TRUE if the message is set, FALSE otherwise.
 */
function acquia_lift_nav_message_is_set() {
  return isset($_SESSION['acquia_lift_personalize_set_message']);
}


/**
 * Unsets the instructional navigation message.
 */
function acquia_lift_unset_nav_message() {
  unset($_SESSION['acquia_lift_personalize_set_message']);
}
