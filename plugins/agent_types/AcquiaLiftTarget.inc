<?php
/**
 * @file
 * Provides an agent for doing fixed targeting.
 */

class AcquiaLiftTarget extends PersonalizeAgentBase implements PersonalizeExplicitTargetingInterface, PersonalizeAgentGoalInterface {

  /**
   * Implements PersonalizeAgentInterface::create().
   */
  public static function create($agent_data) {
    $status = personalize_agent_get_status($agent_data->machine_name);
    return new self($agent_data->machine_name, $agent_data->label, $agent_data->data, $status, !empty($agent_data->started) ? $agent_data->started : NULL);
  }

  /**
   * Implements PersonalizeExplicitTargetingInterface::explicitTargetingSupportMultiple().
   */
  public static function explicitTargetingSupportMultiple() {
    return PersonalizeExplicitTargetingInterface::EXPLICIT_TARGETING_MULTIPLE_BOTH;
  }

  /**
   * Implements PersonalizeAgentInterface::getType().
   */
  public function getType()
  {
    return 'acquia_lift_target';
  }

  /**
   * Implements PersonalizeAgentInterface::getAssets().
   */
  public function getAssets() {
    $settings = acquia_lift_target_default_js_settings($this->machineName);
    $settings_array = array(
      'type' => 'setting',
      'data' => array(
        'acquia_lift_target' => $settings
      )
    );
    $assets = array(
      'js' => array(
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.js' => array('type' => 'file', 'scope' => 'footer', 'defer' => TRUE),
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift_target.js' => array('type' => 'file', 'scope' => 'footer', 'defer' => TRUE),
        $settings_array,
      )
    );
    return $assets;
  }

  /**
   * Implements PersonalizeAgentInterface::convertContextToFeatureString().
   */
  public static function convertContextToFeatureString($name, $value, $is_mutex = FALSE) {
    $value = self::cleanFeatureValue($value);
    return $name . '::' . $value;
  }

  /**
   * Implements PersonalizeAgentInterface::supportsMVTs().
   */
  public function supportsMVTs() {
    return TRUE;
  }

  /**
   * Implements PersonalizeAgentInterface::supportsMultipleDecisionPoints().
   */
  public function supportsMultipleDecisionPoints() {
    // We don't really, but at the time of option set creation, decision point
    // names cannot be set.
    return TRUE;
  }

  /**
   * Implements PersonalizeAgentInterface::errors().
   */
  public function errors() {
    $errors = array();
    $data = $this->getData();
    $is_mvt = isset($data['variation_set_handling']) && $data['variation_set_handling'] == ACQUIA_LIFT_DECISION_MULTIVARIATE;
    // If there are tests involved, first check that there's at least one goal
    // set up.
    $goals = personalize_goal_load_by_conditions(array('agent' => $this->getMachineName()));
    $option_sets = personalize_option_set_load_by_agent($this->getMachineName());
    if ($is_mvt) {
      $needs_goals = TRUE;
      $test_agent_name = acquia_lift_get_mvt_name_for_agent($this->getMachineName());
      if (empty($goals)) {
        // MVTs that have been fully implemented have their goals and option sets
        // set up directly on the test agent.
        $goals = personalize_goal_load_by_conditions(array('agent' => $test_agent_name));
      }
      if (empty($option_sets)) {
        $option_sets = personalize_option_set_load_by_agent($test_agent_name);
      }
    }
    else {
      $agent_data = personalize_agent_load($this->getMachineName());
      // The way we verify the agent is different depending on whether the
      // targeting structure has been implemented or not.
      $implemented = empty($agent_data->data['lift_targeting']);
      if ($implemented) {
        $nested = acquia_lift_get_nested_tests($agent_data);
        $needs_goals = !empty($nested);
      }
      else {
        $needs_goals = FALSE;
        // Check if there's an audience with more than one variation.
        foreach ($agent_data->data['lift_targeting'] as $audience => $variations) {
          if (count($variations) > 1) {
            $needs_goals = TRUE;
            break;
          }
        }
      }
    }
    $variations_assigned = FALSE;
    if (empty($data['lift_targeting'])) {
      module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
      $targeting_os = acquia_lift_get_option_set_for_targeting($this->getMachineName());
      $audiences = acquia_lift_get_structure_from_targeting($targeting_os);
    }
    else {
      $audiences = $data['lift_targeting'];
    }
    foreach ($audiences as $targeting) {
      if (!empty($targeting)) {
        $variations_assigned = TRUE;
      }
    }

    if ($needs_goals && empty($goals)) {
      $errors[] = t('You have not added any goals yet. Please add a goal in the <a href="!why_url">"Why" section</a>.', array(
        '!why_url' => url('admin/structure/personalize/manage/' . $this->getMachineName() . '/goals'),
      ));
    }
    if (empty($option_sets)) {
      $errors[] = t('You have not added any variation sets yet. Please add a variation set in the <a href="!what_url">"What" section</a>.', array(
        '!what_url' => url('admin/structure/personalize/manage/' . $this->getMachineName() . '/variations'),
      ));
    }
    if (!$variations_assigned) {
      $errors[] = t('You have not assigned any variations to audiences. Please add variations for targeting in the <a href="!who_url">"Who" section</a>.', array(
        '!who_url' => url('admin/structure/personalize/manage/' . $this->getMachineName() . '/targeting'),
      ));
    }
    $errors = array_merge($errors, $this->getMultipleOptionSetErrors($option_sets));
    return $errors;
  }

  /**
   * Determine the error messaging to show based on multiple variation set
   * handling configuration for the current agent.
   *
   * @param $option_sets
   *   The option sets to check based on this agent's configuration.
   * @return array
   *   An array of error messages to display which will be empty if there are
   *   no errors.
   */
  public function getMultipleOptionSetErrors($option_sets) {
    $data = $this->getData();
    $is_mvt = isset($data['variation_set_handling']) && $data['variation_set_handling'] == ACQUIA_LIFT_DECISION_MULTIVARIATE;

    $count_os = count($option_sets);
    $errors = array();
    if ($is_mvt) {
      if ($count_os < 2) {
        $errors[] = t('A multi-variate test requires at least two option sets. Please add another variation set in the <a href="!what_url">"What" section</a>.', array(
          '!what_url' => url('admin/structure/personalize/manage/' . $this->getMachineName() . '/variations'),
        ));
      }
    }
    if (count($option_sets) > 1) {
      // If there are multiple option sets, check that they all have the same number
      // of options.
      if (!acquia_lift_target_validate_lock_step($option_sets)) {
        // @todo Get proper text for this.
        $errors[] = t('You have multiple variation sets with different numbers of variations. The personalization cannot be started until all variation sets have the same number of variations. Fix this in the <a href="!what_url">%section section</a>.', array(
          '!what_url' => url('admin/structure/personalize/manage/' . $this->getMachineName() . '/variations'),
          '%section' => t('What'),
        ));
      }
    }
    return $errors;
  }

  /**
   * Implements PersonalizeAgentGoalInterface::useClientSideGoalDelivery().
   *
   * @return boolean
   */
  public function useClientSideGoalDelivery() {
    return TRUE;
  }

  /**
   * Implements PersonalizeAgentGoalInterface::sendGoal().
   */
  public function sendGoal($goal_name, $value = NULL) {
    return;
  }
}
