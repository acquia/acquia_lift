<?php

/**
 * Class AcquiaLiftReportBase
 *
 * This class is provided as a base reporting class for displaying report markup
 * regardless of where reporting data is coming from. It provides functions for
 * building out the markup of overview and experiment reports, as well as some
 * helper methods for formatting numbers, and methods for getting and setting
 * the report configuration. Reporting classes should extend this base class
 * and use it to generate the markup, while implementing their own logic for
 * getting and manipulating the actual reporting data.
 */
abstract class AcquiaLiftReportBase {

  /**
   * The Acquia Lift agent instance for reporting on.
   *
   * @var AcquiaLiftAgent
   */
  protected $agent;

  /**
   * The confidence measure for determining statistical significance.
   */
  protected $confidence_measure = 95;

  /**
   * The configuration options for the report.
   *
   * @var array;
   */
  protected $reportConfig;


  /**
   * Constructs an AcquiaLiftReport object
   *
   * @param PersonalizeAgentInterface $agent
   *   The agent the report is for.
   *
   * @param array $options
   *   An array of report options.
   */
  function __construct(PersonalizeAgentInterface $agent, $options) {
    $this->agent = $agent;
    $confidence_measure = $options['confidence_measure'];
    if ($confidence_measure < 0) {
      $confidence_measure = 0;
    }
    if ($confidence_measure > 100) {
      $confidence_measure = 100;
    }
    $this->confidence_measure = $confidence_measure;
    unset($options['confidence_measure']);
    $this->setReportConfiguration($options);
  }

  /**
   * Gets the confidence_measure property.
   */
  public function getConfidenceMeasure() {
    return $this->confidence_measure;
  }

  /**
   * Returns the configuration for the reports.
   *
   * @return array
   *   An associative array that can have the following keys:
   *   - 'today_only' Boolean indicating whether reports should only be for the
   *     current day.
   *   - 'date_from' The start date for the reports
   *   - 'date_to' The end date for the reports
   *   - 'decision_name' The specific decision the report is for
   *   - 'machine_name' The agent machine name
   *   - 'confidence_measure' The confidence measure to use in the reports
   *   - 'goal' A particular goal to restrict reports to
   *   -  conversion_metric: Metric to display within the conversion report. One
   *      of 'rate' (conversion rate) or 'value' (conversion value).
   */
  protected function getReportConfiguration() {
    return $this->reportConfig;
  }

  /**
   * Converts the passed in options to configuration for all reports.
   *
   * @param $options
   *   An array of options for the report.
   *   - decision: (Optional) decision point name to limit results.
   *   - start: (Optional) start date for report, defaults to agent start.
   *   - end: (Optional) end date for report, defaults to current date.
   *   - goal: (Optional) goal to show in report, defaults to all.
   *   - conversion_metric: (Optional) metric to show in report, defaults to
   *     'rate'.
   */
  protected function setReportConfiguration($options) {
    $decision_name = empty($options['decision']) ? NULL : $options['decision'];
    $date_from = empty($options['start']) ? NULL : $options['start'];
    $date_to = empty($options['end']) ? NULL : $options['end'];

    $machine_name = $this->agent->getMachineName();
    // Campaign start and end time could be different from the report dates and
    // are used for the "total time running" info.
    $start_time = $this->agent->getStartTime();
    $end_time = time();
    $date_from = empty($date_from) ? date('Y-m-d', $start_time) : $date_from;

    $date_to = empty($date_to) ? date('Y-m-d', $end_time) : $date_to;

    $confidence_measure = $this->getConfidenceMeasure();
    // Convert the confidence measure from a percentage to a value between
    // 0 and 1 as expected by the Lift API.
    $confidence_measure /= 100;

    $agent_data = $this->agent->getData();
    // Set the configuration for the reports.
    $this->reportConfig = array(
      'campaign_start' => $start_time,
      'date_from' => $date_from,
      'date_to' => $date_to,
      'decision_name' => $decision_name,
      'machine_name' => $machine_name,
      'confidence_measure' => $confidence_measure,
      'goal' => empty($options['goal']) ? NULL : $options['goal'],
      'conversion_metric' => empty($options['conversion_metric']) ? 'rate' : $options['conversion_metric']
    );
  }

}

