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
  $card_label = isset($element['#title']) ? $element['#title'] : '';
  $header_children = isset($element['header']) ? element_children($element['header']) : array();
  unset($element['footer']);

  // Set default values
  $element['#collapsible'] = isset($element['#collapsible']) ? $element['#collapsible'] : TRUE;
  $element['#collapsed'] = isset($element['#collapsed']) ? $element['#collapsed'] : TRUE;
  $element['#sortable'] = isset($element['#sortable']) ? $element['#sortable'] : FALSE;
  $element['#attributes']['data-card-collapsible'] = $element['#collapsible'] ? 'true' : 'false';
  $element['#attributes']['data-card-collapsed'] = $element['#collapsed'] ? 'true' : 'false';
  $element['#attributes']['data-card-footer-visible'] = empty($footer_children) ? 'false' : 'true';
  $element['#attributes']['data-card-sortable'] = $element['#sortable'] ? 'true' : 'false';

  $element['#attributes']['class'][] = 'el-card';
  drupal_add_library('acquia_lift', 'acquia_lift.card');
  $html = '';
  $html .= '<div ' . drupal_attributes($element['#attributes']) . '>';
  $html .= '<div class="el-card__header">';
  $html .= '<div class="el-card__dragger"></div>';
  $html .= '<h2 class="el-card__title">' . $card_label . '</h2>';
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
  $html .= drupal_render_children($element);
  if (!empty($footer_children)) {
    $html .= '<div class="el-card__footer">' . drupal_render($footer_children) . '</div>';
  }
  $html .= '</div>';
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
    $attributes['id'] = $element['#id'];
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
  drupal_add_library('acquia_lift', 'acquia_lift.inputs');
  $html = '';
  $html .= '<div class="acquia-lift-revealing-input clearfix">';
  $html .= $variables['element']['#children'];
  $html .= '</div>';
  return $html;
}
