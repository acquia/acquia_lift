<?php

/**
 * Class for Acquia Lift Learn Reports.
 */

class AcquiaLiftLearnReport extends AcquiaLiftReportBase {

  /**
   * The report data that has been extracted from the report data source.
   *
   * @var array
   */
  protected $reportData;

  /**
   * The results aggregated across all dates
   *
   * @var array
   */
  protected $aggregatedResults;

  /**
   * The aggregated results interpreted for confidence and lift.
   *
   * @var array
   */
  protected $interpretedResults;


  const NA_STRING = '--';

  const MIN_DECISION_THRESHOLD = 20;

  /**
   * An array of probabilities corresponding to quantiles.
   * @var array
   */
  protected static $probabilities = array(.25,.20,.15,.10,.05,.025,.02,.01,.005,.0025,.001,.0005);

  /**
   * The normal quantiles for the probabilities stores in the probabilities
   * variable above.
   *
   * @var array
   */
  protected static $normal_quantiles = array(0.674,0.841,1.036,1.282,1.645,1.960,2.054,2.326,2.576,2.807,3.091,3.291);

  /**
   * Constructs an AcquiaLiftReport object
   *
   * @param PersonalizeAgentInterface $agent
   *   The agent the report is for.
   *
   * @param AcquiaLiftReportDataSourceInterface $report_data_src
   *   The source for the report data.
   */
  function __construct(PersonalizeAgentInterface $agent, $options, AcquiaLiftLearnReportDataSourceInterface $report_data_src) {
    parent::__construct($agent, $options);
    $this->reportDataSrc = $report_data_src;
  }

  /**
   * Gets the render array for the main reporting page.
   *
   * @return array
   *   An array of output to pass to drupal_render.
   */
  public function getCampaignReports() {
    return $this->buildCampaignReports(
      $this->getSessionCount(),
      $this->getGoalCount(),
      $this->getDailyData(),
      $this->getAggregatedData(),
      $this->getOverallConfidence(),
      $this->getOverallWinner()
    );
  }

  /**
   * Returns a render array for the experimental portion of the reports.
   *
   * @return array
   */
  public function getExperimentReports() {
    $config = $this->getReportConfiguration();
    $report_name = t('All goals');
    $goal = NULL;
    if (!empty($config['goal'])) {
      $goal = $config['goal'];
      $actions = visitor_actions_get_actions();
      // If the action still exists, use the label, otherwise just use the goal
      // machine name.
      if (isset($actions[$goal])) {
        $report_name = $actions[$goal]['label'];
      }
      else {
        $report_name = $goal;
      }
    }

    $reports = $this->buildExperimentReports(
      $report_name,
      $this->getDailyData(),
      $this->getAggregatedData(),
      $this->getOverallConfidence(),
      $this->getOverallWinner());
    return $reports;
  }

  /**
   * Retrieves the report data from the data source and stores it locally.
   *
   * @return array
   *   An array of report data.
   */
  protected function getReportData() {
    if (empty($this->reportData)) {
      $config = $this->getReportConfiguration();
      try {
        $this->reportData = $this->reportDataSrc->getReportForDateRange($this->agent->getMachineName(), $config['date_from'], $config['date_to']);

      }
      catch (Exception $e) {
        drupal_set_message(t('There was a problem retrieving reporting data for this campaign, please try again later.'), 'error');
      }
    }
    return $this->reportData;
  }

  /**
   * Retrieves the aggregated results for the campaign.
   *
   * @return array
   */
  protected function getAggregateResults() {
    if (empty($this->aggregatedResults)) {
      $this->aggregatedResults = array(
        'totals' => array(),
        'totals_by_var' => array()
      );
      $data = $this->getReportData();
      if (!isset($data['results'])) {
        return $this->aggregatedResults;
      }
      $aggregated_per_variation = array();
      foreach ($data['results'] as $result) {
        if (empty($result['results'])) {
          continue;
        }
        $choice_id = $result['decision_id'];
        $aggregated_per_variation[$choice_id] = array();
        foreach ($result['results'] as $single_day) {
          $keys = array_keys($single_day);
          foreach ($keys as $key) {
            if (!is_numeric($single_day[$key])) {
              continue;
            }
            if (!isset($aggregated_per_variation[$choice_id][$key])) {
              $aggregated_per_variation[$choice_id][$key] = 0;
            }
            $aggregated_per_variation[$choice_id][$key] += $single_day[$key];
          }
        }
      }
      $this->aggregatedResults = array(
        'totals' => array(
          'plays' => $data['total_plays'],
          'goals' => $data['total_goals']
        ),
        'totals_by_var' => $aggregated_per_variation
      );
    }
    return $this->aggregatedResults;
  }

  /**
   * Returns the total number of sessions (plays) for this campaign.
   *
   * @return int
   */
  protected function getSessionCount() {
    $data = $this->getAggregateResults();
    return isset($data['totals']['plays']) ? $data['totals']['plays'] : 0;
  }

  /**
   * Returns the total number of goals received for this campaign.
   *
   * @return int
   */
  protected function getGoalCount() {
    $data = $this->getAggregateResults();
    return isset($data['totals']['goals']) ? $data['totals']['goals'] : 0;
  }

  /**
   * Gets the per-day confidence report from the report data source.
   *
   * @return array
   *   An array of per-day reports.
   */
  protected function getDailyData() {
    $report_data = $this->getReportData();
    if (!isset($report_data['results'])) {
      $this->hasData = FALSE;
      return array();
    }
    $data = array();
    foreach ($report_data['results'] as $i => $result) {
      if (empty($result['results'])) {
        continue;
      }
      $count = $goals = $val = 0;
      foreach ($result['results'] as $single_day) {
        $count += $single_day['total_plays_explore'];
        $goals += $single_day['total_goals_explore'];

        $val += $single_day['total_goals_value_explore'];
        $rate = $count > 0 ? ($goals / $count) : 0;
        $mean = $count > 0 ? ($val / $count) : 0;
        $goal_value = $goals ? floor($val / $goals) : 1;
        // Calculate confidence bounds for conversion rate.
        $sd = $count ? sqrt(($rate*(1-$rate))/($count)) : 0;
        // We want a 90% confidence interval, which means we need the 95th
        // quantile, given the two tails of the distribution.
        $quantile = self::$normal_quantiles[4];
        $upper = $goal_value * ($rate + ($quantile * $sd));
        $lower = $goal_value * ($rate - ($quantile * $sd));

        $ts = strtotime($single_day['timestamp']);
        $date = date('Y-m-d', $ts);
        $option_label = $this->getOptionLabelForChoice($result['decision_id']);
        $data[] = array(
          'option_id' => $result['decision_id'],
          'option_label' => $option_label,
          'goals' => $single_day['total_goals_explore'],
          'count' => $single_day['total_plays_explore'],
          'date' => $date,
          'timestamp' => $ts,
          'conversion' => $this->formatReportPercentage($rate),
          'conversion_value' => $this->formatReportNumber($mean),
          'estimated_value' => $this->formatReportNumber($mean, TRUE, 4),
          'margin_error' => $this->formatReportNumber(($upper - $lower) / 2, TRUE, 4),
          'counter' => $i,
          'control' => $i === 0,
        );
      }
    }
    return $data;
  }

  /**
   * Returns the per-variation confidence report data aggregated over dates.
   *
   * @return array
   *   An array of report data keyed by option id.
   */
  protected function getAggregatedData() {
    if (empty($this->interpretedResults)) {
      $this->interpretedResults = array(
        'confidence' => FALSE,
        'winner' => NULL,
        'data' => array()
      );
      $report_data = $this->getAggregateResults();
      if (empty($report_data['totals_by_var'])) {
        $this->hasData = FALSE;
        return array();
      }
      $data = array();
      $i = $total_means = $mean_control = $var_control = $num_control = 0;
      $num_variations = count($report_data['totals_by_var']);
      // We'll be getting a confidence interval for the conversion rate of each
      // variation based on the configured confidence measure. We need to convert
      // this into a p-value to look up the correct normal quantile.
      $desired_p_value = (1 - ($this->getConfidenceMeasure() / 100)) / 2;
      if (in_array($desired_p_value, self::$probabilities)) {
        $pos = array_search($desired_p_value, self::$probabilities);
      }
      else {
        $pos = 0;
        while($desired_p_value < self::$probabilities[$pos]) {
          $pos++;
        }
      }
      $quantile = self::$normal_quantiles[$pos];
      $min_decisions_reached = TRUE;
      foreach ($report_data['totals_by_var'] as $choice_id => $results) {
        $count = $results['total_plays_explore'];
        if ($count < self::MIN_DECISION_THRESHOLD) {
          $min_decisions_reached = FALSE;
        }
        $goals = $results['total_goals_explore'];
        $val = $results['total_goals_value_explore'];
        $rate = $count > 0 ? ($goals / $count) : 0;
        $mean = $count > 0 ? ($val / $count) : 0;
        // @todo We are making an assumption here that all goals received had the
        //   same value. In the rare cases where there are multiple goals of
        //   different values, this will result in an inaccurate calculation of
        //   the variance. We will be fixing this in our next gen reporting.
        $goal_value = $goals ? floor($val / $goals) : 0;
        // Calculate confidence bounds for conversion rate.
        $sd = $count ? sqrt(($rate*(1-$rate))/($count)) : 0;
        $upper = $goal_value * ($rate + ($quantile * $sd));
        $lower = $goal_value * ($rate - ($quantile * $sd));
        $variance = $count > 1 ? (($goals * pow($goal_value - $mean, 2)) + (($count-$goals) * pow($mean,2))) / ($count-1) : 0;
        $option_label = $this->getOptionLabelForChoice($choice_id);
        $data[$choice_id] = array(
          'counter' => $i,
          'option_id' => $choice_id,
          'option_label' => $option_label,
          'goals' => $goals,
          'count' => $count,
          'mean' => $mean,
          'variance' => $variance,
          'conversion' => $this->formatReportPercentage($rate),
          'estimated_value' => $this->formatReportNumber($mean, TRUE, 4),
          'estimated_higher' => $this->formatReportNumber($upper, TRUE, 4),
          'estimated_lower' => $this->formatReportNumber($lower, TRUE, 4),
          'margin_error' => $this->formatReportNumber(($upper - $lower) / 2, TRUE, 4),
          'control' => $i === 0,
          'confidence' => self::NA_STRING,
          'lift_default' =>  self::NA_STRING,
          'lift_random' => self::NA_STRING,
        );
        $total_means += $mean;
        $i++;
      }
      $control_variation = reset($data);
      // Now we can do some comparisons of the data.
      $rand_mean = $total_means / $num_variations;

      foreach ($data as $choice_id => &$values) {
        // Calculate lift over random.
        if ($rand_mean) {
          $lift_random = (($values['estimated_value'] - $rand_mean)/$rand_mean) * 100;
        }
        else {
          $lift_random = 0;
        }
        $values['lift_random'] = $values['count'] > 0 ? $lift_random : self::NA_STRING;
        // It only makes sense to calculate lift over default if there has been
        // at least one decision for each variation and we have a non-zero value
        // for the mean of the default.
        if (!$values['control'] && $control_variation['count'] && $control_variation['mean'] && $values['count']) {
          $lift_default = ($values['mean'] - $control_variation['mean'])/$control_variation['mean'];
          $values['lift_default'] = $this->formatReportPercentage($lift_default , TRUE, TRUE, 2);
        }
        // Only calculate confidence if our minimum decision threshold was reached
        // for each variation.
        if ($min_decisions_reached) {
          $other_variations = $data;
          unset($other_variations[$choice_id]);
          $this->doComparisons($values, $other_variations);
        }
      }
      $this->interpretedResults['data'] = $data;
    }
    return $this->interpretedResults['data'];
  }

  /**
   * Runs a t-test comparing $variation to each of the $other_variations.
   *
   * If $variation beats each of the $other_variations, then the 'overAllWinner'
   * property is set to this variation's id, and the overallConfidence property
   * is set to the lowest confidence value found during the comparisons.
   *
   * @param $variation
   *   The variation to compare against each other variations.
   * @param $other_variations
   *   The other variations to compare against. If the $variation param is not
   *   the control for this test, then the first of the $other_variations is
   *   assumed to be the control.
   */
  protected function doComparisons(&$variation, $other_variations) {
    $control_variation = $variation['control'] ? $variation : reset($other_variations);
    // Keep track of how many other variations this variation beats.
    $number_beaten = $overall_confidence = 0;
    foreach ($other_variations as $var) {
      $confidence = self::getConfidence(self::getTScore($variation['count'], $var['count'], $variation['mean'], $var['mean'], $variation['variance'], $var['variance']), self::getDGFree($variation['count'], $var['count'], $variation['variance'], $var['variance']));
      // If the comparison is against the control variation, then this confidence
      // metric is what we display as "chance to beat control".
      if ($var['option_id'] == $control_variation['option_id']) {
        $variation['confidence'] = $this->formatReportPercentage($confidence);
      }
      if ($confidence < ($this->confidence_measure / 100)) {
        // No point doing anymore comparisons, this is not a winner.
        break;
      }
      // If this is the lowest confidence value we've seen, then this is
      // what we'll show as the overall confidence for the test, if this
      // variation turns out to be the overall winner.
      if (empty($overall_confidence) || $confidence < $overall_confidence) {
        $overall_confidence = $confidence;
      }
      $number_beaten++;
    }
    if ($number_beaten === count($other_variations)) {
      $this->interpretedResults['winner'] = $variation['option_id'];
      $this->interpretedResults['confidence'] = $overall_confidence;
    }
  }

  /**
   * Gets the overall confidence of the report, based on aggregated data.
   *
   * @return bool
   *   TRUE if the report has overall confidence, FALSE otherwise.
   */
  public function getOverallConfidence() {
    if (empty($this->interpretedResults)) {
      $this->getAggregatedData();
    }
    return $this->interpretedResults['confidence'];
  }

  /**
   * Gets the overall winner of the campaign, based on aggregated data.
   *
   * @return int|NULL
   *   An integer representing the index of the winner variation, or NULL if
   *   there isn't one.
   */
  public function getOverallWinner() {
    if (empty($this->interpretedResults)) {
      $this->getAggregatedData();
    }
    return $this->interpretedResults['winner'];
  }

  /**
   * Does a t-test of the difference between two variations.
   * @param $nx
   *   The count for the first variation
   * @param $ny
   *   The count for the variation to compare against.
   * @param $meanx
   *   The mean of the first variation.
   * @param $meany
   *   The mean of the variation being compared with.
   * @param $varx
   *   The variance of the first variation.
   * @param $vary
   *   The variance of the variation being compared with.
   * @return float|int
   *   A test statistic for the difference.
   */
  protected function getTScore($nx, $ny, $meanx, $meany, $varx, $vary) {
    if ($meanx == 0 || $nx == 0 || $ny == 0) {
      return 0;
    }
    $num = ($meanx - $meany);
    $den = sqrt(($varx/$nx) + ($vary/$ny));
    return $den ? $num/$den : 0;
  }

  /**
   * Calculates the degrees of freedom for comparing 2 variations.
   *
   * @param $nx
   *   The count of the first variation
   * @param $ny
   *   The count of the second variation.
   * @param $varx
   *   The variance of the first variation.
   * @param $vary
   *   The variance of the second variation.
   * @return int
   *   An integer representing the degrees of freedom.
   */
  protected static function getDGFree($nx, $ny, $varx, $vary) {
    if ($ny < 2 || $nx < 2) {
      return 0;
    }
    $top = pow(($varx/$nx) + ($vary/$ny), 2);
    $bottom = (pow($varx/$nx, 2) / ($nx-1)) + (pow($vary/$ny, 2) / ($ny-1));
    return $bottom? round($top/$bottom) : 0;
  }


  /**
   * Given a test statistic and number of degrees of freedom, returns a confidence
   * value.
   *
   * @param $tscore
   *   The test statistic
   * @param $df
   *   Number of degrees of freedom
   * @return float|int
   *   A number between 0 and 1 indicating the confidence that the test staticstic
   *   is significant.
   */
  public static function getConfidence($tscore, $df) {
    if ($df < 1) {
      return 0;
    }
    if ($df > 1000) {
      // If we have more than 1000 degrees of freedom, we just use the normal
      // quantiles.
      $pvalue = self::getPValue($tscore, self::$normal_quantiles);
    }
    else {
      $quantiles = self::getTQuantiles();
      $dfs = array_keys($quantiles);
      if (in_array($df, $dfs)) {
        $pvalue = self::getPValue($tscore, $quantiles[$df]);
      }
      else {
        $i = count($quantiles) - 1;
        while ($df <= $dfs[$i]) {
          $i--;
        }
        $pvalue = self::getPValue($tscore, $quantiles[$dfs[$i]]);
      }
    }
    if ($pvalue === NULL) {
      return .5;
    }
    return 1 - (2 * $pvalue);
  }

  /**
   * Returns arrays of t-quantiles for various degrees of freedom.
   *
   * See http://bcs.whfreeman.com/ips6e/content/cat_050/ips6e_table-d.pdf.
   *
   * @return array
   *   An array whose keys are numbers of degrees of freedom and whose values are
   *   arrays of t-quantiles correponding to the probabilities defined in
   */
  protected static function getTQuantiles() {
    return array(
      1 => array(1.000,1.376,1.963,3.078,6.314,12.71,15.89,31.82,63.66,127.3,318.3,636.6),
      2 => array(0.816,1.061,1.386,1.886,2.920,4.303,4.849,6.965,9.925,14.09,22.33,31.60),
      3 => array(0.765,0.978,1.250,1.638,2.353,3.182,3.482,4.541,5.841,7.453,10.21,12.92),
      4 => array(0.741,0.941,1.190,1.533,2.132,2.776,2.999,3.747,4.604,5.598,7.173,8.610),
      5 => array(0.727,0.920,1.156,1.476,2.015,2.571,2.757,3.365,4.032,4.773,5.893,6.869),
      6 => array(0.718,0.906,1.134,1.440,1.943,2.447,2.612,3.143,3.707,4.317,5.208,5.959),
      7 => array(0.711,0.896,1.119,1.415,1.895,2.365,2.517,2.998,3.499,4.029,4.785,5.408),
      8 => array(0.706,0.889,1.108,1.397,1.860,2.306,2.449,2.896,3.355,3.833,4.501,5.041),
      9 => array(0.703,0.883,1.100,1.383,1.833,2.262,2.398,2.821,3.250,3.690,4.297,4.781),
      10 => array(0.700,0.879,1.093,1.372,1.812,2.228,2.359,2.764,3.169,3.581,4.144,4.587),
      11 => array(0.697,0.876,1.088,1.363,1.796,2.201,2.328,2.718,3.106,3.497,4.025,4.437),
      12 => array(0.695,0.873,1.083,1.356,1.782,2.179,2.303,2.681,3.055,3.428,3.930,4.318),
      13 => array(0.694,0.870,1.079,1.350,1.771,2.160,2.282,2.650,3.012,3.372,3.852,4.221),
      14 => array(0.692,0.868,1.076,1.345,1.761,2.145,2.264,2.624,2.977,3.326,3.787,4.140),
      15 => array(0.691,0.866,1.074,1.341,1.753,2.131,2.249,2.602,2.947,3.286,3.733,4.073),
      16 => array(0.690,0.865,1.071,1.337,1.746,2.120,2.235,2.583,2.921,3.252,3.686,4.015),
      17 => array(0.689,0.863,1.069,1.333,1.740,2.110,2.224,2.567,2.898,3.222,3.646,3.965),
      18 => array(0.688,0.862,1.067,1.330,1.734,2.101,2.214,2.552,2.878,3.197,3.611,3.922),
      19 => array(0.688,0.861,1.066,1.328,1.729,2.093,2.205,2.539,2.861,3.174,3.579,3.883),
      20 => array(0.687,0.860,1.064,1.325,1.725,2.086,2.197,2.528,2.845,3.153,3.552,3.850),
      21 => array(0.686,0.859,1.063,1.323,1.721,2.080,2.189,2.518,2.831,3.135,3.527,3.819),
      22 => array(0.686,0.858,1.061,1.321,1.717,2.074,2.183,2.508,2.819,3.119,3.505,3.792),
      23 => array(0.685,0.858,1.060,1.319,1.714,2.069,2.177,2.500,2.807,3.104,3.485,3.768),
      24 => array(0.685,0.857,1.059,1.318,1.711,2.064,2.172,2.492,2.797,3.091,3.467,3.745),
      25 => array(0.684,0.856,1.058,1.316,1.708,2.060,2.167,2.485,2.787,3.078,3.450,3.725),
      26 => array(0.684,0.856,1.058,1.315,1.706,2.056,2.162,2.479,2.779,3.067,3.435,3.707),
      27 => array(0.684,0.855,1.057,1.314,1.703,2.052,2.158,2.473,2.771,3.057,3.421,3.690),
      28 => array(0.683,0.855,1.056,1.313,1.701,2.048,2.154,2.467,2.763,3.047,3.408,3.674),
      29 => array(0.683,0.854,1.055,1.311,1.699,2.045,2.150,2.462,2.756,3.038,3.396,3.659),
      30 => array(0.683,0.854,1.055,1.310,1.697,2.042,2.147,2.457,2.750,3.030,3.385,3.646),
      40 => array(0.681,0.851,1.050,1.303,1.684,2.021,2.123,2.423,2.704,2.971,3.307,3.551),
      50 => array(0.679,0.849,1.047,1.299,1.676,2.009,2.109,2.403,2.678,2.937,3.261,3.496),
      60 => array(0.679,0.848,1.045,1.296,1.671,2.000,2.099,2.390,2.660,2.915,3.232,3.460),
      80 => array(0.678,0.846,1.043,1.292,1.664,1.990,2.088,2.374,2.639,2.887,3.195,3.416),
      100 => array(0.677,0.845,1.042,1.290,1.660,1.984,2.081,2.364,2.626,2.871,3.174,3.390),
      1000 => array(0.675,0.842,1.037,1.282,1.646,1.962,2.056,2.330,2.581,2.813,3.098,3.300),
    );
  }

  /**
   * Given a test statistic and an array of quantiles corresponding to the correct
   * number degrees of freedom, returns the p-value associated with the test
   * statistic.
   *
   * @param $tscore
   *   The test statistic to get a p-value for.
   * @param $quantiles
   *   The relevant array of quantiles to search through.
   * @return float
   *   The estimated p-value.
   */
  protected static function getPValue($tscore, $quantiles) {
    $pvalues = self::$probabilities;
    if ($tscore < $quantiles[0]) {
      // We don't even have 50% confidence. Return NULL to signify we don't have
      // a significant p-value.
      return NULL;
    }
    $max = count($pvalues) - 1;
    $lower = $max;
    while($tscore <= $quantiles[$lower]) {
      $lower--;
    }
    if ($lower == $max) {
      return $pvalues[$lower];
    }
    else {
      $upper = $lower + 1;
      // Calculate the distance from the lower statistic to the actual statistic
      // as a fraction of the distance between the upper and lower statistics we
      // have p-values for.
      $distance = ($tscore - $quantiles[$lower]) / ($quantiles[$upper] - $quantiles[$lower]);
      // Now we can get a more accurate p-value by adding on this distance.
      return $pvalues[$lower] + ($distance * ($pvalues[$upper] - $pvalues[$lower]));
    }

  }

  protected function getOptionLabelForChoice($choice_id) {
    $option_label = $choice_id;
    // @todo Once we have an easy way of getting the parent agent we can look
    //   up the option label in the parent option set.
/*    $option_sets = personalize_option_set_load_by_agent($this->agent->getMachineName());
    foreach ($option_sets as $option_set) {
      foreach ($option_set->options as $option) {
        if ($option['option_id'] == $choice_id) {
          $option_label = $option['option_label'];
          break 2;
        }
      }
    }*/
    return $option_label;
  }
}