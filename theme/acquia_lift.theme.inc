<?php

/**
 * @file acquia_lift.theme.inc
 * Provides theme functions for Acquia Lift.
 */


/**
 * Returns HTML for a personalized form element.
 *
 * @param $variables
 *   An associative array containing:
 *   - element: A render element representing the form element.
 *
 * @ingroup themeable
 */
function theme_acquia_lift_personalizable_field_form($variables) {

  $element = $variables['element'];

  // Sort items according to '_weight' (needed when the form comes back after
  // preview or failed validation).
  $items = array();
  foreach (element_children($element) as $key) {
    if ($key === 'add_more') {
      $add_more_button = &$element[$key];
      // Convert the form-submit to a button so that it is more styleable with // css. If markup changes are still needed by a theme, allow for a more
      // specific theme_html_tag__personalize_button() override.
      $add_more_button['#theme'] = 'html_tag__personalize_button';
      $add_more_button['#tag'] = 'button';
      $add_more_button['#attributes']['id'] = $add_more_button['#id'];
      $add_more_button['#attributes']['class'][] = 'form-submit';
      $add_more_button['#attributes']['class'][] = 'personalize-field-add';
      $add_more_button['#attributes']['title'] = t('Add an option');
      $add_more_button['#value'] = t('Add an option');
      unset($add_more_button['#theme_wrappers']);
    }
    else {
      $element[$key]['_weight']['#theme_wrappers'][] = 'acquia_lift_personalize_field_weight_field_wrapper';
      $items[] = &$element[$key];
    }
  }
  usort($items, '_field_sort_items_value_helper');

  $rendered_items = '<div class="personalize-field-options">';

  foreach ($items as $key => $item) {
    $class = 'personalize-field' . ($key == 0 ? ' personalize-field-visible' : '');
    $rendered_items .= '<div class="' . $class . '">' . drupal_render($item) . '</div>';
  }
  $rendered_items .= '</div>';

  $children = drupal_render($add_more_button);
  $children .= $rendered_items;

  $output = '<div class="form-item">';

  // If #title is not set, we don't display any label or required marker.
  if (!isset($element['#title'])) {
    $element['#title_display'] = 'none';
  }
  $prefix = isset($element['#field_prefix']) ? '<span class="field-prefix">' . $element['#prefix'] . '</span> ' : '';
  $suffix = isset($element['#field_suffix']) ? ' <span class="field-suffix">' . $element['#suffix'] . '</span>' : '';

  switch ($element['#title_display']) {
    case 'before':
    case 'invisible':
      $output .= ' ' . theme('form_element_label', $variables);
      $output .= ' ' . $prefix . $children . $suffix . "\n";
      break;

    case 'after':
      $output .= ' ' . $prefix . $children . $suffix;
      $output .= ' ' . theme('form_element_label', $variables) . "\n";
      break;

    case 'none':
    case 'attribute':
      // Output no label and no required marker, only the children.
      $output .= ' ' . $prefix . $children . $suffix . "\n";
      break;
  }

  $output .= $element['#description'] ? '<div class="description">' . $element['#description'] . '</div>' : '';
  $output .= '</div>';

  return $output;
}

/**
 * Theme wrapper for the personalize option weight field.
 */
function theme_acquia_lift_personalize_field_weight_field_wrapper($variables) {
  $attributes = array(
    'class' => array('element-invisible'),
    'aria-hidden' => 'true',
  );
  return '<div'. drupal_attributes($attributes) . '>' . $variables['element']['#children'] . '</div>';
}

/**
 * Theme function for the in-context personalization controls.
 */
function theme_acquia_lift_edit_mode_personalize_in_context_links($variables) {
  $elements = $variables['elements'];
  $launch = drupal_render($elements['launch']);
  $cancel = drupal_render($elements['cancel']);
  return '<div' . drupal_attributes($elements['#attributes']) . '>' . $launch . ' ' . t('or') . ' ' . $cancel . '</div>';
}

/**
 * Theme function to output links for filtering reports by date.
 *
 * @ingroup themeable
 */
function theme_acquia_lift_date_filter_links($variables) {
  $element = $variables['element'];
  $output = '<div>Filter by date: ';

  $rendered = array();
  foreach ($element['links'] as $link) {
    $rendered[] = drupal_render($link);
  }
  $output .= implode (' | ', $rendered);

  $output .= '</div>';
  return $output;
}

/**
 * Theme function to output links for filtering by feature,
 *
 * @ingroup themeable
 */
function theme_acquia_lift_feature_filter_links($variables) {
  $element = $variables['element'];
  $output = '<div>Filter by feature: ';

  $rendered = array();
  foreach ($element['links'] as $link) {
    $rendered[] = drupal_render($link);
  }
  $output .= implode (' | ', $rendered);

  $output .= '</div>';
  return $output;
}

/**
 * Theme function to output tablinks for jQuery UI style tabs.
 *
 * @ingroup themeable
 */
function theme_acquia_lift_report_tablinks($vars) {

  $output = '<ul>';
  foreach ($vars['tabset']['tablinks'] as $i => $tab) {
    if (is_array($tab)) {
      $output .= '<li>'. drupal_render($tab) .'</li>';
    }
  }
  $output .= '</ul>';
  return $output;
}

/**
 * Theme function to output content for jQuery UI style tabs.
 *
 * @ingroup themeable
 */
function theme_acquia_lift_tabbed_report($variables) {
  $element = $variables['element'];
  $output = '<div '. drupal_attributes($element['#options']['attributes']) .'>';
  $output .= drupal_render($element['tabs']);

  if (isset($element['active'])) {
    $output .= drupal_render($element['active']);
  }
  else {
    foreach ($element['divs'] as $div) {
      $output .= drupal_render($div);
    }
  }

  $output .= '</div>';
  return $output;
}
