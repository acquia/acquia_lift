<?php
/**
 * @file
 * Provides a context plugin for Acquia Lift Profiles
 */

namespace Drupal\acquia_lift_profiles\AcquiaLiftProfilesContext;

use Drupal\acquia_lift_profiles\AcquiaLiftProfilesAPI;

class AcquiaLiftProfilesContext extends PersonalizeContextBase {

  /**
   * Implements PersonalizeContextInterface::create().
   */
  public static function create(PersonalizeAgentInterface $agent, $selected_context) {
    try {
      /** @var \Drupal\acquia_lift_profiles\AcquiaLiftProfilesAPI $acquia_lift_profiles */
      $acquia_lift_profiles = \Drupal::service('acquia_lift_profiles');
      return new self($agent, $selected_context, $acquia_lift_profiles);
    }
    catch (\Exception $e) {
      \Drupal::logger('Acquia Lift Profiles')->notice($e->getMessage(), array());
      return NULL;
    }
  }

  /**
   * Implements PersonalizeContextInterface::getOptions().
   */
  public static function getOptions() {
    $options = array();
    try {
      /** @var \Drupal\acquia_lift_profiles\AcquiaLiftProfilesAPI $acquia_lift_profiles */
      $acquia_lift_profiles = \Drupal::service('acquia_lift_profiles');
      $segments = $acquia_lift_profiles->getSegments();
      if (!empty($segments)) {
        foreach ($segments as $segment) {
          $options[$segment] = array(
            'name' => $segment,
            'group' => 'Acquia Lift Profiles segments',
            'cache_type' => 'local',
            'cache_expiration' => 'none',
          );
        }
      }
    }
    catch (Exception $e) {
      drupal_set_message($e->getMessage(), 'error');
    }
    return $options;
  }

  /**
   * Constructs an AcquiaLiftProfilesContext object.
   *
   * @param $selected_context
   * @param AcquiaLiftProfilesAPI $acquia_lift_profiles_api
   */
  public function __construct(PersonalizeAgentInterface $agent, $selected_context, AcquiaLiftProfilesAPI $acquia_lift_profiles_api) {
    parent::__construct($agent, $selected_context);
    $this->acquia_lift_profilesAPI = $acquia_lift_profiles_api;
  }

  /**
   * Implements PersonalizeContextInterface::getPossibleValues().
   */
  public function getPossibleValues($limit = FALSE) {
    $possible_values = array();
    $options = $this->getOptions();
    foreach ($options as $name => $info) {
      $possible_values[$name] = array(
        'value type' => 'boolean',
        'friendly name' =>  t('Acquia Lift Profiles segment: @segment', array('@segment' => $name)),
        'on_label' => t('Yes'),
        'off_label' => t('No')
      );
    }
    if ($limit) {
      $possible_values = array_intersect_key($possible_values, array_flip($this->selectedContext));
    }
    return $possible_values;
  }
}
