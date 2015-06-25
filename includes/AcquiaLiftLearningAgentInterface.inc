<?php
/**
 * @file
 * Provides an interface for Acquia Lift learning agents.
 */
interface AcquiaLiftLearningAgentInterface {

  /**
   * Builds out all the reports for a campaign.
   *
   * @param $options
   *   An associative array of options controlling the report behavior with the
   *   following keys:
   *   - decision_name: (Optional) The decision to get conversion report for.
   *   - goal: (Optional) The goal for limiting the report, defaults to all goals.
   *   - start: (Optional) The start time for report data in Y-m-d format.
   *     Defaults to agent start.
   *   - end: (Optional) The end time for report data in Y-m-d format.  Defaults
   *     to the current date.
   * @return array
   *   A render array containing all of the report markup.
   */
  public function getCampaignReports($options);

  /**
   * Retrieves a conversion report.
   *
   * @param $options
   *   An array of report options.
   *   - decision_name: (Optional) The decision to get conversion report for.
   *   - goal: (Optional) The goal for limiting the report, defaults to all goals.
   *   - start: (Optional) The start time for report data in Y-m-d format.
   *     Defaults to agent start.
   *   - end: (Optional) The end time for report data in Y-m-d format.  Defaults
   *     to the current date.
   * @return array
   *   A renderable array.
   */
  public function getExperimentReports($options);
}
