<?php
/**
 * @file acquia_lift.wizard.theme.inc
 * Provides theme functions for Acquia Lift's version of the campaign creation
 * wizard workflow.
 */

/**
 * Theme function to show a card element.
 *
 * @param $variables
 *   An associate array of variables including the "element" to be themed.
 */
function theme_acquia_lift_card($variables) {
  $element = $variables['element'];
  $footer_children = empty($element['footer']) ? array() : element_children($element['footer']);
  foreach($footer_children as $key) {
    $footer_elements[] = $element['footer'][$key];
  }
  $card_label = isset($element['#title']) ? $element['#title'] : '';
  $header_children = isset($element['header']) ? element_children($element['header']) : array();
  $title_attributes = isset($element['#title_attributes']) ? $element['#title_attributes'] : array();
  $card_flag = isset($element['#flag']) ? $element['#flag'] : '';
  $card_flag_attributes = array('class' => array('el-card__flag'));
  if (isset($element['#flag_visible']) && $element['#flag_visible'] === FALSE) {
    $card_flag_attributes['class'][] = 'is-hidden';
  }
  if (isset($element['card_flag_attributes'])) {
    $card_flag_attributes = array_merge_recursive($card_flag_attributes, $element['card_flag_attributes']);
  }
  unset($element['footer']);

  // Set default values
  $element['#collapsible'] = isset($element['#collapsible']) ? $element['#collapsible'] : TRUE;
  $element['#collapsed'] = isset($element['#collapsed']) ? $element['#collapsed'] : TRUE;
  $element['#sortable'] = isset($element['#sortable']) ? $element['#sortable'] : FALSE;
  $element['#disabled'] = isset($element['#disabled']) ? $element['#disabled'] : FALSE;
  $element['#attributes']['data-card-collapsible'] = $element['#collapsible'] ? 'true' : 'false';
  $element['#attributes']['data-card-collapsed'] = $element['#collapsed'] ? 'true' : 'false';
  $element['#attributes']['data-card-footer-visible'] = empty($footer_children) ? 'false' : 'true';
  $element['#attributes']['data-card-sortable'] = $element['#sortable'] ? 'true' : 'false';
  $element['#attributes']['data-card-enabled'] = $element['#disabled'] ? 'false' : 'true';
  $element['#attributes']['class'][] = 'el-card';
  // The ID will also be on the form wrapper for the card so we need to update
  // it to make it unique.
  if (!empty($element['#attributes']['id'])) {
    $element['#attributes']['id'] .= '-card';
  }
  drupal_add_library('acquia_lift', 'acquia_lift.card');
  $html = '';
  $html .= '<div ' . drupal_attributes($element['#attributes']) . '>';
  $html .= '<div class="el-card__header">';
  $html .= '<div class="el-card__dragger"></div>';

  $title_attributes['class'][] = 'el-card__title';
  $html .= '<h2 ' . drupal_attributes($title_attributes) . '>' . $card_label . '</h2>';
  if (!empty($header_children)) {
    $html .= '<ul class="el-card__actions">';
    foreach ($header_children as $action) {
      $element['header'][$action]['#attributes']['class'][] = 'el-card__action__link';
      $html .= '<li class="el-card__action">' . drupal_render($element['header'][$action]) . '</li>';
    }
    $html .= '</ul>';
  }
  $html .= '</div>';
  $html .= '<div class="el-card__content">';
  if (!empty($card_flag)) {
    $html .= '<div ' . drupal_attributes($card_flag_attributes) . '><span class="el-card__flag__label">' . t('Status') . ':</span> <span class="el-card__flag__content">' . $card_flag . '</span></div>';
  }
  $html .= drupal_render_children($element);
  $html .= '</div>'; // Close content.
  if (!empty($footer_elements)) {
    $html .= '<div class="el-card__footer">' . drupal_render($footer_elements) . '</div>';
  }
  $html .= '</div>'; // Close card.
  return $html;
}

/**
 * Theme function to convert a submit button to an "add" button within the
 * wizard process
 */
function theme_acquia_lift_add_card_button(&$variables) {
  $element = &$variables['element'];
  $element['#attributes']['class'][] = 'acquia-lift-wizard-add';
  $element['#button_type'] = 'submit';
  $html = '';
  $html .= '<div class="acquia-lift-wizard-add-wrapper">';
  $html .= theme('button', $element);
  $html .= '</div>';
  return $html;
}

/**
 * Theme function to return a radio button that appears as a list of options.
 */
function theme_acquia_lift_radio_list($variables) {
  $element = $variables['element'];
  $attributes = array();
  if (isset($element['#id'])) {
    $attributes['id'] = $element['#id'] . '-radio-list';
  }
  $attributes['class'] = 'form-radios acquia-lift-radio-list';
  if (!empty($element['#attributes']['class'])) {
    $attributes['class'] .= ' ' . implode(' ', $element['#attributes']['class']);
  }
  if (isset($element['#attributes']['title'])) {
    $attributes['title'] = $element['#attributes']['title'];
  }
  return '<div' . drupal_attributes($attributes) . '>' . drupal_render_children($element) . '</div>';
}

/**
 * Theme wrapper function to wrap an individual radio button with it's image
 * and container.
 */
function theme_acquia_lift_radio_list_item($variables) {
  $element = $variables['element'];
  $html = '';
  $html .= '<div class="acquia-lift-radio-list-item clearfix">';
  $html .= '<div class="acquia-lift-radio-list-item-image">';
  if (!empty($element['#image'])) {
    $html .= $element['#image'];
  }
  $html .= '</div>';
  $html .= '<div class="acquia-lift-radio-list-item-holder">' . (empty($element['#children']) ? '' : $element['#children']) . '</div>';
  $html .= '</div>';
  return $html;
}

/**
 * Theme wrapper function to wrap an input as a revealing input.
 */
function theme_acquia_lift_revealing_input($variables) {
  if (empty($variables['element']['#children'])) {
    return '';
  }
  if (!empty($variables['element']['#disabled'])) {
    unset($variables['element']['#theme_wrappers']);
    return drupal_render($variables['element']);
  }
  $classes = array('acquia-lift-revealing-input');
  if (!isset($variables['element']['#fix']) || $variables['element']['#fix'] == TRUE) {
    $classes[] = 'clearfix';
  };
  drupal_add_library('acquia_lift', 'acquia_lift.inputs');
  $html = '';
  $html .= '<div class="' . implode(' ', $classes) . '">';
  $html .= $variables['element']['#children'];
  $html .= '</div>';
  return $html;
}

/**
 * Theme function to output a list of variations as displays.
 *
 * @param $variables
 *   array of theme variables including:
 *   - items: an array of variations to display.  Each item in the array
 *     will be an array of variations for that display.  If there is only
 *     one variation set, then there will only be one variation per item key.
 */
function theme_acquia_lift_variations_list($variables) {
  $items = $variables['items'];
  $displays = array();
  $max_variations = 0;
  foreach ($items as $variations) {
    $max_variations = max($max_variations, count($variations));

    $displays[] = theme('item_list', array(
      'items' => $variations,
    ));
  }
  $list_classes = array('acquia-lift-variations-list');
  if ($max_variations > 1) {
    $list_classes[] = 'acquia-lift-variations-lockstep';
  }
  return theme('item_list', array(
    'items' => $displays,
    'attributes' => array(
      'class' => $list_classes,
    ),
  ));
}

/**
 * Theme function to output the option set and option name when showing
 * variation listings.
 * @param $variables
 *   - option_set: the label for the option set
 *   - option: the label for the option
 */
function theme_acquia_lift_single_variation($variables) {
  $html = '';
  $html = '<span class="acquia-lift-variation">';
  if (empty($variables['option_set'])) {
    $html .= $variables['option'];
    $html .= '</span>';
    return $html;
  }
  $html .= $variables['option'] . '&nbsp;';
  $html .= '<span class="acquia-lift-variation-set">(';
  $html .= $variables['option_set'];
  $html .= ')</span>';
  $html .= '</span>';
  return $html;
}

/**
 * Section level help for the campaign wizard.
 *
 * @param $variables
 *   Array of theme variables including the "element" to be themed.
 *   The element may include properties:
 *   - #image: An option image to display
 *   - #alt: The alt text for the above image
 *   - #text: The help text to display
 */
function theme_acquia_lift_wizard_section_help($variables) {

  $element = $variables['element'];
  // Currently just supports a single image and simple html text.
  $help_image = isset($element['#image']) ? $element['#image'] : '';
  $help_alt = isset($element['#alt']) ? $element['#alt'] : t('Help image for this section');
  $help_text = isset($element['#text']) ? $element['#text'] : '';
  $trigger_text = t('Help?');
  $collapsed = isset($element['#collapsed']) ? $element['#collapsed'] : TRUE;
  $container_classes = array('clearfix');
  if ($collapsed) {
    $container_classes[] = 'is-collapsed';
  }
  else {
    $container_classes[] = 'is-expanded';
  }

  $html = '';
  $html .= '<div id="acquia-lift-campaign-wizard-section-help" class="' . implode(' ', $container_classes) . '">';
  $html .= '<a class="acquia-lift-section-help-trigger" title="' . $trigger_text . '">' . $trigger_text . '</a>';
  $html .= '<div class="acquia-lift-section-help-expanded">';
  $html .= '<div class="acquia-lift-section-help clearfix">';
  if (!empty($help_image)) {
    $html .= theme('image', array(
      'path' => $help_image,
      'alt' => $help_alt,
      'attributes' => array(
        'class' => 'acquia-lift-section-help',
      ),
    ));
  }
  $html .= '<div class="acquia-lift-section-help">' . $help_text . '</div>';
  $html .= '</div>';
  $element['dismiss']['#attributes']['class'][] = 'acquia-lift-submit-button';
  $element['dismiss']['#attributes']['class'][] = 'action-item-primary-active';
  $element['dismiss']['#attributes']['class'][] = 'acquia-lift-section-help-collapse';
  $html .= drupal_render($element['dismiss']);
  $html .= '</div>';
  return $html;
}
