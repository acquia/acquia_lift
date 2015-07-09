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
  $is_admin = TRUE;
  if (!path_is_admin(current_path()) && preg_match('/^admin\/structure\/block\/demo\//', current_path()) == 0) {
    $is_admin = FALSE;
  }

  // Attach client-side controls for managing personalized content.
  _acquia_lift_navigation_attach_assets($page['page_top'], $is_admin);

  // Necessary ctools integration for modal windows.  These are used only for
  // administrative functionality.
  if (user_access('manage personalized content')) {
    ctools_include('modal');
    ctools_include('ajax');
    ctools_modal_add_js();

    // Have to add styling here.  When added as part of a library it is always
    // added before ctools and therefore cannot override styles.
    $page['page_top']['#attached']['library'][] = array('acquia_lift', 'acquia_lift.modal');
    ctools_add_css('acquia_lift.ctools.modal', 'acquia_lift');

    // Load data about active campaigns.
    $settings['acquia_lift']['campaigns'] = acquia_lift_get_campaign_details();
    // Load data about custom defined visitor actions.
    $actions = visitor_actions_custom_load_multiple();
    $settings['acquia_lift']['customActions'] = $actions;

    $settings['acquia_lift']['dom_selector_ignore'] = acquia_lift_generate_ignore_selector();
    $settings['acquia_lift']['edit_in_context_html_strip'] = variable_get('acquia_lift_html_context_strip', 1);

    // Add any pending messages from the query string.
    $params = drupal_get_query_parameters();
    if (!empty($params['liftpm']) && strstr($params['liftpm'], '|') !== FALSE) {
      list($type, $details) = explode('|', $params['liftpm']);
      if ($type === 'new_block') {
        $settings['acquia_lift']['pendingMessage'][] = t('Created the new %block_title personalized block. The block will not appear on your website until you add the block to a region on the !blocks page.', array(
          '%block_title' => $details,
          '!blocks' => l('Structure > Blocks', 'admin/structure/blocks'),
        ));
      }
    }
    drupal_add_js($settings, 'setting');
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
  $element['#attributes']['class'] = $classes;
  $element['#attributes']['data-placeholder'] = t('Choose a context...');
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
 * Generates a selector for all of the regions of this page to ignore when
 * selecting elements from the DOM.
 */
function acquia_lift_generate_ignore_selector() {
  global $theme;
  $classes = array();
  $all_regions = system_region_list($theme, REGIONS_ALL);
  $visible_regions = system_region_list($theme, REGIONS_VISIBLE);
  $invisible_regions = array_diff($all_regions, $visible_regions);
  foreach ($invisible_regions as $region_name => $region_label) {
    $classes[] = drupal_region_class($region_name);
  }
  return $classes;
}

/**
 * =======================================================================
 * S H A R E D  U I
 * =======================================================================
 */

/**
 * Helper function to return a list of option set type metadata.
 *
 * @return array
 *   An array for each type of option set available with the following keys:
 *   - title: A title for the type of option set
 *   - description: A brief description
 *   - logo: a themed image that can be used to represent the option set type
 *   - path: the link to create an option set of this type
 */
function acquia_lift_option_set_types_ui() {
  $path = drupal_get_path('module', 'acquia_lift');
  return array(
    'block' => array(
      'title' => t('Drupal blocks'),
      'description' => t('This variation style allows you to display personalized content in a Drupal block, wherever it may appear on your website.'),
      'logo' => theme('image', array(
        'path' => $path . '/images/variation-type-block.png',
        'alt' => t('Drupal block'),
        'title' => t('Select this option to create a variation set from Drupal content blocks.'),
      )),
      'path' => 'admin/structure/personalize/variations/personalize-blocks/add',
    ),
    'element' => array(
      'title' => t('Webpage elements'),
      'description' => t('This variation style allows you to select an item on a webpage and modify its HTML to create new variations of the content.'),
      'logo' => theme('image', array(
        'path' => $path . '/images/variation-type-element.png',
        'alt' => t('Webpage element'),
        'title' => t('Select this option to create a variation set from web page elements.'),
      )),
      'path' => 'admin/structure/personalize/variations/personalize-elements/add',
    ),
  );
}

/**
 * Helper function to return a list of goal type metadata.
 *
 * @return array
 *   An array for each goal type available wit the following keys:
 *   - title: A title for the type of goal
 *   - description: A brief description
 *   - logo: a themed image that can be used to represent the goal type
 */
function acquia_lift_goal_types_ui() {
  $path = drupal_get_path('module', 'acquia_lift');
  return array(
    'existing' => array(
      'title' => t('Predefined goal'),
      'description' => t('Visitor actions that are already defined, such as signing in or submitting a form.'),
      'path' => 'admin/structure/acquia_lift/goal/add/existing',
      'logo' => theme('image', array(
        'path' => $path . '/images/goal-type-predefined.png',
        'alt' => t('Pre-existing goal'),
        'title' => t('Select this option to create a goal from a pre-existing visitor action.'),
      )),
    ),
    'element' => array(
      'title' => t('New element goal'),
      'description' => t('Visitor actions that involve clicking a link, hovering over a link, or submitting a form.'),
      'path' => 'admin/structure/visitor_actions/add-in-context',
      'logo' => theme('image', array(
        'path' => $path . '/images/goal-type-element.png',
        'alt' => t('Element goal'),
        'title' => t('Select this option to create an element goal.'),
      ))
    ),
    'page' => array(
      'title' => t('New page goal'),
      'description' => t('Visitor actions for a specific webpage that involve viewing, scrolling to the bottom of, or staying for a set timeframe.'),
      'path' => 'admin/structure/acquia_lift/goal/add/page',
      'logo' => theme('image', array(
        'path' => $path . '/images/goal-type-page.png',
        'alt' => t('Page goal'),
        'title' => t('Select this option to create a page goal.'),
      ))
    ),
  );
}

/**
 * =======================================================================
 *  F U N C T I O N S  B A S E D  O N  N A V B A R
 * These functions are only used/useful when navbar is not included.
 * =======================================================================
 */
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
