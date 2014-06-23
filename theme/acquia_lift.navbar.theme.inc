<?php

/**
 * @file acquia_lift.navbar.theme.inc
 * Provides theme functions for Acquia Lift unified navigation bar.
 *
 * These functions were based on the theme functions for the navbar module.
 */

/**
 * Returns HTML that wraps the administration navbar.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties and children of
 *     the tray. Properties used: #children, #attributes and #bar.
 */
function theme_acquia_lift_navbar(&$variables) {
  if (!empty($variables['element']['#children'])) {
    $element = $variables['element'];
    $trays = '';
    foreach (element_children($element) as $key) {
      $trays .= drupal_render($element[$key]['tray']);
    }
    return '<nav' . drupal_attributes($element['#attributes']) . '>'
    . '<div' . drupal_attributes($element['#bar']['#attributes']) . '>'
    . '<h2 class="element-invisible">' . $element['#bar']['#heading'] . '</h2>'
    . $element['#children'] . '</div>' . $trays . '</nav>';
  }
}

/**
 * Implements template_preprocess_HOOK().
 */
function template_preprocess_acquia_lift_navbar_tab_wrapper(&$variables) {
  if (!isset($variables['element']['#wrapper_attributes'])) {
    $variables['element']['#wrapper_attributes'] = array();
  }
  $variables['element']['#wrapper_attributes']['class'][] = 'navbar-tab';
}

/**
 * Returns HTML for a navbar item.
 *
 * This theme function only renders the tab portion of the navbar item. The
 * tray portion will be rendered later.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties and children of
 *     the tray. Property used: tab.
 *
 * @see navbar_pre_render_item().
 * @see theme_navbar().
 */
function theme_acquia_lift_navbar_item(&$variables) {
  return drupal_render($variables['element']['tab']);
}

/**
 * Returns HTML for wrapping a navbar tab.
 *
 * Navbar tabs have a common styling and placement with the navbar's bar area.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties and children of
 *     the tray. Properties used: #children and #attributes.
 */
function theme_acquia_lift_navbar_tab_wrapper(&$variables) {
  if (!empty($variables['element']['#children'])) {
    $element = $variables['element'];
    return '<div' . drupal_attributes($element['#wrapper_attributes']) . '>' . $element['#children'] . '</div>';
  }
}

/**
 * Returns HTML for wrapping a navbar tray.
 *
 * Used in combination with theme_navbar_tab() to create an
 * association between a link tag in the administration bar and a tray.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties and children of
 *     the tray. Properties used: #children, #navbar_identifier and
 *     #attributes.
 */
function theme_acquia_lift_navbar_tray_wrapper(&$variables) {
  if (!empty($variables['element']['#children'])) {
    $element = $variables['element'];
    return '<div' . drupal_attributes($element['#wrapper_attributes']) . '><div class="navbar-lining clearfix">' . $element['#children'] . '</div></div>';
  }
}

/**
 * Returns HTML for prepending a heading to a navbar tray.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties and children of
 *     the tray. Properties used: #children and #heading.
 */
function theme_acquia_lift_navbar_tray_heading_wrapper(&$variables) {
  $element = $variables['element'];
  if (!empty($element['#children'])) {
    $heading = '';
    if (!empty($element['#heading'])) {
      $heading = '<h3 class="navbar-tray-name element-invisible">' . $element['#heading'] . '</h3>';
    }
    return $heading . $element['#children'];
  }
}
