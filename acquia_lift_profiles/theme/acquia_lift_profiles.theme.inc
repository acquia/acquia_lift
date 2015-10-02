<?php

/**
 * @file acquia_lift_profiles.theme.inc
 * Provides theme functions for Acquia Lift Profiles.
 */

/**
 * Theme callback for the form table.
 */
function theme_acquia_lift_profiles_lift_event_value_form_table($variables) {
  $element = $variables['element'];
  $header = array(
    t('Visitor Action'),
    t('Engagement Score'),
    t('Global Value'),
  );

  $rows = array();
  // Traverse each row.
  foreach (element_children($element) as $row_index) {
    $rows[] = array(
      array('data' => $element[$row_index]['#label']),
      array('data' => $element[$row_index]['engagement_score']),
      array('data' => $element[$row_index]['global_value']),
    );
  }

  // Render the table and return.
  return theme('table', array('header' => $header, 'rows' => $rows));
}
