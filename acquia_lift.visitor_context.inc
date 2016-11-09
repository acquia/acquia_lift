<?php

/**
 * @file acquia_lift.visitor_context.inc
 * Provides functions needed for handling visitor contexts.
 */


function _acquia_lift_get_context_options() {
  return array(
    'taxonomy_context' => _acquia_lift_get_taxonomy_context_options()
  );
}

function _acquia_lift_get_taxonomy_context_options() {
  $vocabularies = taxonomy_get_vocabularies();
  $options = array();
  foreach ($vocabularies as $vocabulary) {
    $machine_name = $vocabulary->machine_name;
    $options[$machine_name] = array(
      'name' => $vocabulary->name,
      'machine_name' => $vocabulary->machine_name,
      'group' => 'Taxonomy',
      'id' => $vocabulary->vid,
    );
  }
  return $options;
}
