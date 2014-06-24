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

  $page['page_top']['acquia_lift']['#access'] = user_access('manage personalized content');
  $page['page_top']['acquia_lift']['#attached']['library'][] = array('acquia_lift', 'acquia_lift.personalize');
  // Attach client-side controls for managing personalized content.
  if (_acquia_lift_using_unified_navbar()) {
    $page['page_top']['acquia_lift']['nav']['#type'] = 'acquia_lift_navbar';
    // Make the Acquia Lift tool bar show up after other content.
    $page['page_top']['acquia_lift']['#weight'] = 100;
    $page['page_top']['#sorted'] = FALSE;
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

/**
 * =======================================================================
 *  F U N C T I O N S  B A S E D  O N  N A V B A R
 * These functions are only used/useful when navbar is not included.
 * =======================================================================
 */
/**
 * Gets the library definitions for hook_library.
 */
function acquia_lift_navbar_library() {
  $libraries = array();
  if (module_exists('navbar')) {
    return $libraries;
  }
  $path = drupal_get_path('module', 'acquia_lift');

  $libraries['acquia_lift.unified.navbar'] = array(
    'title' => 'Unified navbar',
    'version' => VERSION,
    'js' => array(
      $path . '/js/acquia_lift.unified_navbar.js' => array(),
    ),
    'css' => array(
      $path . '/css/navbar/navbar.module.css',
      $path . '/css/navbar/navbar.theme.css',
      $path . '/css/navbar/navbar.icons.css',
      $path . '/css/acquia_lift.unified_navbar.css',
    ),
    'dependencies' => array(
      array('acquia_lift', 'modernizr'),
      array('system', 'jquery'),
      array('acquia_lift', 'underscore'),
      array('acquia_lift', 'backbone'),
      array('acquia_lift', 'acquia_lift.navbar.matchmedia'),
      array('system', 'jquery.once'),
      array('acquia_lift', 'acquia_lift.navbar.debounce'),
      array('acquia_lift', 'acquia_lift.navbar.announce'),
      array('acquia_lift', 'acquia_lift.navbar.displace'),
      array('acquia_lift', 'acquia_lift.navbar.menu'),
    ),
  );

  // Only load navbar.overlay if overlay is enabled.
  if (module_exists('overlay')) {
    $libraries['navbar']['dependencies'][] = array('navbar', 'navbar.overlay');
  }

  $libraries['acquia_lift.navbar.menu'] = array(
    'title' => 'Navbar nested accordion menus.',
    'version' => VERSION,
    'js' => array(
      $path . '/js/navbar/navbar.menu.js' => array(),
    ),
    'css' => array(
      $path . '/css/navbar/navbar.menu.css',
    ),
    'dependencies' => array(
      array('system', 'jquery'),
      array('system', 'jquery.once'),
    ),
  );

  // Backport of D8 matchMedia polyfill.
  $libraries['acquia_lift.navbar.matchmedia'] = array(
    'title' => 'window.matchMedia polyfill',
    'website' => 'http://drupal.org/node/1815602',
    'version' => VERSION,
    'js' => array(
      $path . '/js/navbar/matchmedia.js' => array(),
    ),
  );

  // A utility function to limit calls to a function with a given time.
  $libraries['acquia_lift.navbar.debounce'] = array(
    'title' => 'Navbar debounce',
    'version' => VERSION,
    'js' => array(
      $path . '/js/navbar/debounce.js' => array('group' => JS_LIBRARY),
    ),
  );

  // A utility function determine viewport offset distances.
  $libraries['acquia_lift.navbar.displace'] = array(
    'title' => 'Navbar displace',
    'version' => VERSION,
    'js' => array(
      $path . '/js/navbar/displace.js' => array('group' => JS_LIBRARY),
    ),
    'dependencies' => array(
      array('system', 'jquery'),
      array('acquia_lift', 'acquia_lift.navbar.debounce'),
    ),
  );

  // A utility for writing text to a common aria-live region.
  $libraries['acquia_lift.navbar.announce'] = array(
    'title' => 'Navbar announce',
    'version' => VERSION,
    'js' => array(
      $path . '/js/navbar/announce.js' => array('group' => JS_LIBRARY),
    ),
    'dependencies' => array(
      array('acquia_lift', 'acquia_lift.navbar.debounce'),
    ),
  );

  // Override Overlay methods to support displacement.
  $libraries['acquia_lift.navbar.overlay'] = array(
    'title' => 'Overlay method overrides to support D8 viewport displacement.',
    'version' => VERSION,
    'js' => array(
      // Load this file well after Overlay code has loaded.
      $path . '/js/navbar/navbar-overlay.js' => array('weight' => 100),
    ),
    'dependencies' => array(
      array('system', 'jquery'),
      array('acquia_lift', 'acquia_lift.navbar.displace'),
    ),
  );

  // Support Tableheader displacement.
  $libraries['acquia_lift.navbar.tableheader'] = array(
    'title' => 'Tableheader method to support D8 viewport displacement.',
    'version' => VERSION,
    'js' => array(
      // Load this file well after Overlay code has loaded.
      $path . '/js/navbar/navbar-tableheader.js' => array('weight' => 100),
      array(
        'data' => array('tableHeaderOffset' => 'Drupal.navbar.height'),
        'type' => 'setting'
      ),
    ),
    'dependencies' => array(
      array('system', 'jquery'),
      array('acquia_lift', 'acquia_lift.navbar.displace'),
    ),
  );
  return $libraries;
}

/**
 * Builds the unified navbar as a structured array ready for drupal_render().
 *
 * @param array $element
 *   A renderable array
 *
 * @return
 *   A renderable array.
 *
 * @see navbar_pre_render().
 * @see acquia_lift_page_build().
 */
function acquia_lift_navbar_ui_pre_render($element) {
  // Define the breakpoints to switch from vertical to horizontal
  // navbar presentation.
  $breakpoints = array(
    'narrow' => 'only screen and (min-width: 16.5em)',
    'standard' => 'only screen and (min-width: 38.125em)',
    'wide' => 'only screen and (min-width: 50em)',
  );
  // Allow for altering of the breakpoints.
  drupal_alter('acquia_lift_breakpoints', $breakpoints);

  if (!empty($breakpoints)) {
    $element['#attached']['js'][] = array(
      'data' => array(
        'acquia_lift' => array(
          'unified_navbar' => array(
            'breakpoints' => $breakpoints,
          ),
        ),
      ),
      'type' => 'setting',
    );
  }

  // Get the navigation items as defined for the navbar implementation.
  $items = acquia_lift_navbar();
  // Sort the children.
  uasort($items, 'element_sort');

  // Merge in the original navbar values.
  $element = array_merge($element, $items);

  // Render the children.
  $element['#children'] = drupal_render_children($element);

  return $element;
}

/**
 * Provides markup for associating a tray trigger with a tray element.
 *
 * A tray is a responsive container that wraps renderable content. Trays present
 * content well on small and large screens alike.
 *
 * @param array $element
 *   A renderable array.
 *
 * @return
 *   A renderable array.
 *
 * @see navbar_pre_render_item().
 */
function acquia_lift_navbar_ui_pre_render_item($element) {
  // Assign each item a unique ID.
  $id = drupal_html_id('navbar-item');

  // If tray content is present, markup the tray and its associated trigger.
  if (!empty($element['tray'])) {
    // Provide attributes for the tray theme wrapper.
    $attributes = array(
      'id' => $id . '-tray',
      'data-navbar-tray' => $id . '-tray',
      'aria-owned-by' => $id,
    );

    // Merge in module-provided attributes.
    if (!isset($element['tray']['#wrapper_attributes'])) {
      $element['tray']['#wrapper_attributes'] = array();
    }
    $element['tray']['#wrapper_attributes'] += $attributes;
    $element['tray']['#wrapper_attributes']['class'][] = 'navbar-tray';
    // Normally navbar dynamically adds and removes the active class, but it is
    // set statically here because there is only one tray in the unified nav.
    $element['tray']['#wrapper_attributes']['class'][] = 'navbar-active';

    if (!isset($element['tray']['#theme_wrappers'])) {
      $element['tray']['#theme_wrappers'] = array();
    }
    // Add the standard theme_wrapper for trays.
    array_unshift($element['tray']['#theme_wrappers'], 'acquia_lift_navbar_tray_wrapper');
    // If a #heading is provided for the tray, provided a #theme_wrapper
    // function to append it.
    array_unshift($element['tray']['#theme_wrappers'], 'acquia_lift_navbar_tray_heading_wrapper');
  }

  return $element;
}
