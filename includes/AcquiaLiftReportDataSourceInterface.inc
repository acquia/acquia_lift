<?php

interface AcquiaLiftReportDataSourceInterface {
  /**
   * Returns a confidence report for the specified agent and timeframe.
   *
   * @param $agent_name
   *   The name of the agent.
   * @param string $date_start
   *   The start date in the format YYYY-MM-DD or null to use today's date.
   * @param string $date_end
   *   The end date in the format YYYY-MM-DD or null to get a report for just
   *   a single day.
   * @param string $point
   *   An optional decision point to limit the report to.
   * @param array $options
   *   An array of report options to be passed to Acquia Lift where the keys
   *   are the parameter names and the values are the parameter values.
   *   Support includes:
   *     - confidence-measure: The confidence measure to use between 0 and 1.
   *     - aggregated-over-dates: Aggregates the data over the time span or
   *       reports on individual dates
   * @return array
   *   The report as an associative array
   *
   * @throws AcquiaLiftException
   */
  public function getConfidenceReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL, $options = array());

  /**
   * Returns status reports for the specified agents and number of days.
   *
   * @param $agent_names
   *   An array of agent names to return status reports for.
   * @param null $num_days
   *   Number of days to return reports for, or NULL to get the default
   *   14 days.
   *
   * @return array
   *   The report as an associative array
   *
   * @throws AcquiaLiftException
   */
  public function getAgentStatusReport($agent_names, $num_days = NULL);

  /**
   * Returns raw data about the accumulated value of options for the specified
   * agent.
   *
   * @param $agent_name
   *   The name of the agent.
   * @param string $date_start
   *   The start date in the format YYYY-MM-DD or null to use today's date.
   * @param string $date_end
   *   The end date in the format YYYY-MM-DD or null to get a report for just
   *   a single day.
   * @param string $point
   *   An optional decision point to limit the report to.
   * @return array
   *   The report as an associative array
   *
   * @todo Add support for the other optional parameters, i.e. 'confidence-
   *   measure', 'comparison-decision' and 'use-bonferroni'
   *
   * @throws AcquiaLiftException
   */
  public function getRawLearningReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL);
}
