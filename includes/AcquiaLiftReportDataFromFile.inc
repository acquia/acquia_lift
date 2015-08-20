<?php

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
