<?php

/**
 * Factory class to create a report object for Acquia Lift.
 */
class AcquiaLiftReportFactory {

  /**
   * Creates a Report object.
   *
   * @param $agent_instance
   *   The Acquia Lift agent that the reporting is for.
   * @param $api_instance
   *   The Acquia Lift API classes in use.
   * @return
   *   The AcquiaLiftReport class to use.
   */
  public static function create(AcquiaLiftLearningAgentInterface $agent_instance, $api_instance, $options) {
    // Load all reporting-related classes.
    $report_class_files = array(
      'AcquiaLiftReportDataFromFileBase',
      'AcquiaLiftLearnReport',
      'AcquiaLiftLearnReportFromFile',
      'AcquiaLiftReportCacheInterface',
      'AcquiaLiftReportCache',
    );
    foreach ($report_class_files as $file) {
      module_load_include('inc', 'acquia_lift', 'includes/' . $file);
    }

    if (!isset($options['confidence_measure'])) {
      $options['confidence_measure'] = variable_get('acquia_lift_confidence_measure', 95);
    }
    $agent_name = $agent_instance->getMachineName();
    // Check if this agent is set up to read reports from a file instead of
    // calling the API.
    $report_file = variable_get("acquia_lift_report_source_$agent_name", '');
    if (!empty($report_file)) {
      $report_source = new AcquiaLiftLearnReportFromFile($report_file, new AcquiaLiftReportCache());
    }
    else {
      $report_source = $api_instance;
    }
    $report = new AcquiaLiftLearnReport($agent_instance, $options, $report_source);
    return $report;
  }
}