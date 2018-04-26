<?php

/**
 * @file acquia_lift.context.inc
 * Provides functions needed for handling visitor contexts.
 */

/**
 * Gets all options for mappable contexts
 *
 * @return array
 *   An array of options keyed by the context type
 */
function _acquia_lift_get_context_options() {
  $contexts = array(
    'taxonomy_context' => _acquia_lift_get_taxonomy_context_options(),
  );
  return array_filter($contexts);
}

/**
 * Gets all the callbacks for finding the mapped value of a context type.
 * The callback receives the the node and the mapped value.
 *
 * @return array
 *   An array of callables.
 */
function _acquia_lift_get_mapping_callbacks() {
  return array(
    'taxonomy_context' => '_acquia_lift_get_mapped_taxonomy_context'
  );
}

/**
 * Gets all of the node fields that include taxonomy terms.
 *
 * @return array
 *   An array of contextual options that can be used for grouped output.
 */
function _acquia_lift_get_taxonomy_context_options() {
  if (!module_exists('taxonomy')) {
    return array();
  }

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

/**
 * Handles the page context request data.
 *
 * @param $node
 *   (optional) If passed then the taxonomy terms for the selected node
 *   are added to the page taxonomy context.
 * @param bool $primary
 *   True if this is the primary node displayed for the page, false otherwise.
 * @return array
 *   Returns the taxonomy context for the page.
 */
function _acquia_lift_get_page_context($node = NULL, $primary = FALSE) {
  $page_context = &drupal_static(__FUNCTION__);
  if (!isset($page_context)) {
    $page_context = array();
  }

  // Add the content type of the page if this is the primary node displayed.
  if ($primary && empty($page_context['post_id'])) {
    $page_context['content_type'] = $node->type;
    $page_context['content_title'] = $node->title;
    $page_context['published_date'] = $node->created;
    $page_context['post_id'] = $node->nid;
    $page_context['content_uuid'] = isset($node->uuid) ? $node->uuid : '';
    $page_context['page_type'] = 'node page';
    $page_context = array_merge_recursive($page_context, _acquia_lift_get_mapped_context($node));
  }

  return $page_context;
}

/**
 * Gets the mapped context for the current node including field mappings and
 * udf mappings.
 *
 * @param $node
 *   The node currently be displayed
 * @return array
 *   An array of contexts to be included with the page context.
 */
function _acquia_lift_get_mapped_context($node) {
  $mapped_context = array();
  $mapping_callbacks = _acquia_lift_get_mapping_callbacks();
  // Field Mappings include content section, keywords, persona, etc.
  $field_mappings = variable_get('acquia_lift_field_mappings', array());
  foreach($field_mappings as $type => $mapping) {
    list($context_type, $context) = explode(ACQUIA_LIFT_ADMIN_SEPARATOR, $mapping);
    if (!array_key_exists($context_type, $mapping_callbacks)) {
      continue;
    }
    $mapped_value = call_user_func($mapping_callbacks[$context_type], $node, $context);
    if (!is_null($mapped_value)) {
      $mapped_context[$type] = $mapped_value;
    }
  }
  // UDF Mappings include keys for person, touch, event, etc.
  $udf_mappings = variable_get('acquia_lift_udf_mappings', array());
  foreach($udf_mappings as $type => $mapping) {
    foreach($mapping as $udf => $mapping_context) {
      list ($context_type, $context) = explode(ACQUIA_LIFT_ADMIN_SEPARATOR, $mapping_context);
      if (!array_key_exists($context_type, $mapping_callbacks)) {
        continue;
      }
      $mapped_value = call_user_func($mapping_callbacks[$context_type], $node, $context);
      if (!is_null($mapped_value)) {
        $mapped_context[$udf] = $mapped_value;
      }
    }
  }
  return $mapped_context;
}

/**
 * Gets the list of taxonomy terms applied for a particular node and a
 * particular taxonomy vocabulary.
 * @param $node
 *   The node that is being displayed
 * @param string $vocabulary_machine_name
 *   The machine name of the vocabulary
 * @return null if no mapping, a string of terms if mapping exists.
 */
function _acquia_lift_get_mapped_taxonomy_context($node, $vocabulary_machine_name) {
  if (empty($node) || empty($vocabulary_machine_name)) {
    return null;
  }
  $mapped = array();
  $results = db_query('SELECT tid FROM {taxonomy_index} WHERE nid = :nid', array(':nid' => $node->nid));
  $terms = taxonomy_term_load_multiple($results->fetchCol());
  foreach($terms as $term) {
    if ($term->vocabulary_machine_name !== $vocabulary_machine_name) {
      continue;
    }
    $mapped[] = $term->name;
  }
  return implode(',', $mapped);
}
