<?php
/**
 * @file
 * Provides an agent type for Acquia Lift
 */

class AcquiaLiftContext extends PersonalizeContextBase {

  /**
   * Implements PersonalizeContextInterface::create().
   */
  public static function create(PersonalizeAgentInterface $agent, $selected_context) {
    try {
      $lift_api = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()));
      return new self($agent, $selected_context, $lift_api);
    }
    catch (AcquiaLiftException $e) {
      watchdog('Acquia Lift', 'Unable to instantiate Acquia Lift Agent');
      return NULL;
    }
  }

  /**
   * Implements PersonalizeContextInterface::getOptions().
   */
  public static function getOptions() {
    $options = array();
    try {
      $transform_options = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()))->getTransformOptions();
      // @todo Once more visitor_context plugins have been developed it may
      //   make sense to define some of these categories at a higher
      //   level and have multiple plugins add to them. We could define
      //   the categories as constants in the Personalize module.
      $category_tags = array(
        'Census' => t('Location demographics'),
        'Time' => t('Time related'),
        'Geo' => t('Location'),
        'User-Agent' => t('Browser/device related'));
      foreach ($transform_options['providers'] as $option) {
        // Only options with providesFeatures set to TRUE can be used for
        // automatic targeting.
        if (isset($option['providesFeatures']) && $option['providesFeatures']) {
          if (isset($option['tags'])) {
            if (in_array('Referer', $option['tags'])) {
              // We don't want to include any of the Referer-related tags as we
              // provide this functionality via the Personalize URL Context module.
              continue;
            }
            // Find the first category that exists in this option's tags
            // and use that as the group.
            foreach (array_keys($category_tags) as $tag) {
              if (in_array($tag, $option['tags'])) {
                $group = $category_tags[$tag];
                break;
              }
            }
          }

          $options[$option['code']] = array(
            'name' => $option['name']
          );
          if (isset($group)) {
            $options[$option['code']]['group'] = $group;
          }
        }
      }
    }
    catch (AcquiaLiftException $e) {
      drupal_set_message($e->getMessage(), 'error');
    }

    return $options;
  }

  /**
   * Implements PersonalizeContextInterface::allowedFromAgent().
   */
  public static function allowedFromAgent(StdClass $agent) {
    return !empty($agent->plugin) && $agent->plugin === 'acquia_lift';
  }


  /**
   * Constructs an AcquiaLiftContext object.
   *
   * @param $selected_context
   * @param AcquiaLiftAPI $lift_api
   */
  public function __construct(PersonalizeAgentInterface $agent, $selected_context, AcquiaLiftAPI $lift_api) {
    parent::__construct($agent, $selected_context);
    $this->liftAPI = $lift_api;
  }

  /**
   * Implements PersonalizeAgentInterface::getPossibleValues().
   */
  public function getPossibleValues() {
    if ($this->agent->getType() != 'acquia_lift') {
      return array();
    }
    $possible_values = array();
    try {
      $possible_values = $this->liftAPI->getPossibleValues($this->agent->getMachineName());
    }
    catch (AcquiaLiftException $e) {
      drupal_set_message($e->getMessage(), 'error');
    }
    return $possible_values;
  }
}
