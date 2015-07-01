<?php

/**
 * Class AcquiaLiftReportDataFromFileBase
 *
 * Reads reports from a file
 */
class AcquiaLiftReportDataFromFileBase {

  /**
   * The path to the file that acts as the reporting source.
   *
   * @var string
   */
  protected $fileLocation;

  /**
   * An array of reports to act as a request length storage for the reports.
   *
   * @var array
   */
  protected $reports;

  public function __construct($file_location, AcquiaLiftReportCacheInterface $cache = NULL) {
    $this->fileLocation = $file_location;
    $this->cache = $cache;
  }

  /**
   * Retrieves a particular report for the specifed agent.
   *
   * @param $agent_name
   *   The name of the agent to retrieve a report for.
   * @param $report_name
   *   (Optional) The name of the report to retrieve.
   * @return array
   *   An array representing the report.
   */
  public function getReport($agent_name, $report_name = NULL) {

    // First see if we already have the reports in memory.
    if ($this->reports) {
      if (!empty($report_name)) {
        return isset($this->reports[$report_name]) ? $this->reports[$report_name] : array();
      }
      return $this->reports;
    }

    // Check the cache if available
    if ($this->cache && $reports = $this->cache->getCachedReports($agent_name)) {
      $this->reports = $reports;
      if (!empty($report_name)) {
        return isset($this->reports[$report_name]) ? $this->reports[$report_name] : array();
      }
      return $this->reports;
    }
    // Otherwise resort to reading from a file.
    else {
      $file_name = $this->fileLocation;
      if (strpos($file_name, '://') === FALSE) {
        // If not given full URL, assume it's an absolute path for
        // the current site.
        global $base_url;
        $file_name = $base_url . $file_name;
      }

      if ($str = file_get_contents($file_name)) {
        $parsed = json_decode($str, TRUE);
        $this->reports = isset($parsed['reports']) ? $parsed['reports'] : $parsed;
        if ($this->cache) {
          $this->cache->cacheReports($agent_name, $this->reports);
        }
        if (!empty($report_name)) {
          return isset($this->reports[$report_name]) ? $this->reports[$report_name] : array();
        }
        return $this->reports;
      }
    }
    return array();
  }

}

class AcquiaLiftReportDataFromFile extends AcquiaLiftReportDataFromFileBase implements AcquiaLiftReportDataSourceInterface {

  /**
   * Implements AcquiaLiftReportDataSourceInterface::getTargetingImpactReport().
   */
  public function getTargetingImpactReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL) {
    return $this->getReport($agent_name, 'targeting-impact');
  }

  /**
   * Implements AcquiaLiftReportDataSourceInterface::getAgentStatusReport().
   */
  public function getAgentStatusReport($agent_names, $num_days = NULL) {
    $agent_name = reset($agent_names);
    return $this->getReport($agent_name, 'agent-status');
  }

  /**
   * Implements AcquiaLiftReportDataSourceInterface::getConfidenceReport().
   */
  public function getConfidenceReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL, $options = array()) {
    $report_name = 'confidence';
    if (!empty($options['goal'])) {
      // There are two sample goal reports included in the test file structure.
      // Pick one at random to show for the selected goal.
      $report_name .= '_goal' . rand(1, 2);
    }
    if (isset($options['aggregated-over-dates']) && $options['aggregated-over-dates'] === FALSE) {
      $report_name .= '_detail';
    }
    return $this->getReport($agent_name, $report_name);
  }

  /**
   * Implements AcquiaLiftReportDataSourceInterface::getRawLearningReport().
   */
  public function getRawLearningReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL) {
    return $this->getReport($agent_name, 'raw-learning');
  }
}
