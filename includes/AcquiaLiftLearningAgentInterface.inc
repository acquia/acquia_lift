<?php
/**
 * @file
 * Provides an interface for Acquia Lift learning agents.
 */
interface AcquiaLiftLearningAgentInterface {

  /**
   * Returns an
   * @return mixed
   */
  public static function getAPIInstance();

  /**
   * @param $option_sets
   * @param $new_goals
   * @param array $old_goals
   * @return mixed
   */
  public function getAgentSyncOperations($option_sets, $new_goals, $old_goals = array());

  public function getCampaignReports($options);

  public function getExperimentReports($options);

}
