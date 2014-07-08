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
    // If the user is personalizing content in context, provide a DSM with actions
    // to personalize or cancel out of this 'mode'.
    if (acquia_lift_nav_message_is_set()) {
      drupal_set_message(acquia_lift_edit_mode_personalize_in_context(), 'acquia-lift-personalize');
    }
  }

  // Attach client-side controls for managing personalized content.
  _acquia_lift_navigation_attach_assets($page['page_top'], $is_admin);

  // Necessary ctools integration for modal windows.  These are used only for
  // administrative functionality.
  if (user_access('manage personalized content')) {
    ctools_include('modal');
    ctools_modal_add_js();
    $page['page_top']['#attached']['library'][] = array('acquia_lift', 'acquia_lift.message_box');
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
/**
 * =======================================================================
 *  C A M P A I G N  M A N A G E M E N T  F L O W
 * =======================================================================
 */

/**
 * Create form callback.  Handles including the necessary files in order to
 * load the campaign creation files relevant to AJAX calls.
 *
 * @param bool $ajax
 *   Indicates if form is called with ajax support.
 */
function acquia_lift_create_campaign_callback($ajax) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  return _acquia_lift_modal_get_form(
    t('Create a campaign'),
    'acquia_lift_create_campaign_form',
    'personalize_agent_form',
    $ajax
  );
}

/**
 * Loads the "create a campaign" simplified worklow window.
 *
 * @param bool $ajax
 *   Indicates if the form is called via AJAX support.
 */
function acquia_lift_create_campaign_form($form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  $basic_form = personalize_agent_build_basic_form(NULL, FALSE, array('agent'), TRUE);
  $form['agent'] = $basic_form;
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active'),
    ),
    '#value' => t('Save campaign settings'),
    '#ajax' => array(
      'callback' => 'acquia_lift_create_campaign_form_ajax_submit',
      'wrapper' => 'acquia-lift-create-campaign-form',
      'effect' => 'fade',
    ),
  );
  $form['actions']['reset'] = array(
    '#type' => 'button',
    '#button_type' => 'reset',
    '#value' => t('Cancel'),
    '#attributes' => array(
      'class' => array('acquia-lift-cancel-button'),
    ),
    '#validate' => array(),
    '#limit_validation_errors' => array(),
    '#ajax' => array(
      'callback' => 'acquia_lift_campaign_flow_cancel',
      'wrapper' => 'acquia-lift-create-campaign-form',
      'effect' => 'fade',
    ),
  );
  return $form;
}

function acquia_lift_create_campaign_form_validate($form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  personalize_agent_form_validate($form, $form_state);
}

/**
 * Submit handler to create a campaign from the Acquia Lift UI flow.
 */
function acquia_lift_create_campaign_form_submit($form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  $agent_data = _personalize_agent_from_form_values($form_state['values']['agent']['agent_basic_info']);
  personalize_agent_save($agent_data);
}



/**
 * Helper function to load any form in a modal window.
 *
 * @param $model_title
 *   The title to display in the modal window.
 * @param $form_id
 *   The form id to retrieve for the modal window.
 * @param $alternate_form_id
 *   The alternate form that should be shown in case of no AJAX.
 * @param ...
 *   Any parameters to be passed along to drupal_get_form with the form_id.
 * @param bool $ajax
 *   Indicates if this is called with ajax support.
 * @return mixed
 *   The page to be displayed or the AJAX commands.
 */
function _acquia_lift_modal_get_form() {
  $args = func_get_args();
  $modal_title = array_shift($args);
  $form_id = array_shift($args);
  $alternate_form_id = array_shift($args);
  $ajax = array_pop($args);

  // If not AJAX-enabled, then just return the regular form.
  if (!$ajax) {
    array_unshift($args, $alternate_form_id);
    return call_user_func_array('drupal_get_form', $args);
  }

  ctools_include('ajax');
  ctools_include('modal');

  $form_state = array(
    'ajax' => TRUE,
    'title' => $modal_title,
  );

  $commands = ctools_modal_form_wrapper($form_id, $form_state);
  if (empty($commands)) {
    $commands[] = ctools_modal_command_loading();
    if (!empty($_GET['destination'])) {
      $commands[] = ctools_ajax_command_redirect($_GET['destination']);
    }
    else {
      $commands[] = ctools_modal_command_dismiss();
    }
  }
  $page = array('#type' => 'ajax', '#commands' => $commands);
  ajax_deliver($page);
}

/**
 * AJAX callback to cancel the campaign creation flow.
 */
function acquia_lift_campaign_flow_cancel($form, &$form_state) {
  ctools_include('modal');
  $commands[] = ctools_modal_command_dismiss();
  return $commands;
}

/**
 * Returns an AJAX command to display a message box.
 *
 * @param $message
 *   The string message to display.  This message may include HTML text.
 * @return array
 *   A command array that can be returned via AJAX.
 */
function acquia_lift_command_messagebox($message) {
  if (is_array($message)) {
    $message = drupal_render($message);
  }
  return array(
    'command' => 'acquia_lift_message_box',
    'data' => $message,
  );
}
