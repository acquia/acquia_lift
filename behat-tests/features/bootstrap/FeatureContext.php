<?php

use Drupal\DrupalExtension\Context\RawDrupalContext;
use Behat\Behat\Context\SnippetAcceptingContext;
use Behat\Gherkin\Node\TableNode;
use Behat\Behat\Hook\Scope\AfterStepScope;
use Behat\Mink\Driver\Selenium2Driver;
use Behat\Testwork\Hook\Scope\BeforeSuiteScope;
use Behat\Behat\Hook\Scope\BeforeScenarioScope;
use Behat\Behat\Hook\Scope\AfterScenarioScope;

/**
 * Defines application features from the specific context.
 */
class FeatureContext extends RawDrupalContext implements SnippetAcceptingContext {

  /**
   * Stores the context parameters that are passed in for the test suite.
   * Parameters include default values for:
   *   - temp_path: The path to temporary location where files, such as error
   *     screenshots, can be written. Default value: /tmp
   */
  protected $context_parameters = array();

  /**
   * Stores contexts.
   */
  protected $contexts = array();

  /**
   * Stores campaigns at start of scenario for comparison with those at the end.
   */
  protected $campaigns = array();

  /**
   * Stores visitor actions at start of scenario for comparison at end.
   */
  protected $actions = array();

  /**
   * Initializes context.
   *
   * Every scenario gets its own context instance.
   * You can also pass arbitrary arguments to the
   * context constructor through behat.yml.
   */
  public function __construct($context_parameters) {
    $this->setContextParameters($context_parameters);
  }

  /****************************************************
   *        H O O K S
   ***************************************************/

  /**
   * Perform before scenario actions:
   * - Gather all contexts so they can be reused in the current context.
   *
   * @BeforeScenario
   */
  public function beforeScenario(BeforeScenarioScope $scope) {
    // Gather all contexts.
    $contexts = $scope->getEnvironment()->getContexts();
    foreach ($contexts as $context) {
      $context_class_name = get_class($context);
      $this->contexts[$context_class_name] = $context;
    }
  }

  /**
   * Gets a reference to current campaigns, option sets, goals, etc. for
   * tracking purposes.
   *
   * @BeforeScenario @campaign
   */
  public function beforeScenarioCampaign(BeforeScenarioScope $scope) {
    $this->campaigns = personalize_agent_load_multiple();
    $this->actions = visitor_actions_custom_load_multiple();
  }

  /**
   * Delete any campaigns, option sets, goals, etc. created during the
   * scenario.
   *
   * @AfterScenario @campaign
   */
  public function afterScenarioCampaign(AfterScenarioScope $event) {
    $original_campaigns = $this->campaigns;
    $all_campaigns = personalize_agent_load_multiple(array(), array(), TRUE);
    foreach ($all_campaigns as $name => $campaign) {
      if (!isset($original_campaigns[$name])) {
        $option_sets = personalize_option_set_load_by_agent($name);
        foreach($option_sets as $option_set) {
          personalize_option_set_delete($option_set->osid);
        }
        personalize_agent_delete($name);
      }
    }

    $original_actions = $this->actions;
    $all_actions = visitor_actions_custom_load_multiple(array(), FALSE, TRUE);
    foreach ($all_actions as $name => $action) {
      if (!isset($original_actions[$name])) {
        visitor_actions_delete_action($name);
      }
    }
  }

  /**
   * For javascript enabled scenarios, always wait for AJAX before clicking.
   *
   * @BeforeStep
   */
  public function beforeJavascriptStep($event) {
    $text = $event->getStep()->getText();
    if (preg_match('/(follow|press|click|submit|hover|select)/i', $text)) {
      $this->spinUntilAjaxIsFinished();
    }
  }

  /**
   * For javascript enabled scenarios, always wait for AJAX after clicking.
   *
   * @AfterStep
   */
  public function afterJavascriptStep($event) {
    $text = $event->getStep()->getText();
    if (preg_match('/(follow|press|click|submit|hover|select)/i', $text)) {
      $this->spinUntilAjaxIsFinished();
    }
  }

  /**
   * Take screenshot when step fails.
   * Works only with Selenium2Driver.
   *
   * @AfterStep
   */
  public function takeScreenshotAfterFailedStep(AfterStepScope $scope) {
    if (!$scope->getTestResult()->isPassed()) {
      $driver = $this->getSession()->getDriver();
      if (!($driver instanceof Selenium2Driver)) {
        //throw new UnsupportedDriverActionException('Taking screenshots is not supported by %s, use Selenium2Driver instead.', $driver);
        return;
      }
      $step = $scope->getStep();
      $step_line = $step->getLine();
      $temp_path = $this->getContextParameter('temp_path');
      $filename = $temp_path . '/stepAtLine' . $step_line . '.png';
      $screenshot = $driver->getWebDriverSession()->screenshot();
      file_put_contents($filename, base64_decode($screenshot));
      echo "Saved Screenshot To $filename \n";
      $filename = $temp_path . '/stepAtLine' . $step_line .'.html';
      $source = $driver->getWebDriverSession()->source();
      file_put_contents($filename, $source);
      echo "Saved Source To $filename\n";
    }
  }

  /****************************************************
   *        G I V E N S
   ***************************************************/

  /**
   * @Given /^"(?P<type>[^"]*)" agents:$/
   */
  public function createAgents($type, TableNode $agentsTable) {
    foreach ($agentsTable->getHash() as $agentHash) {
      $agent = (object) $agentHash;
      $agent->plugin = $type;
      $data = array();
      if (!empty($agentHash['url_contexts'])) {
        $data['visitor_context'] = array(
          'querystring_context' => array()
        );
        $contexts = explode(',', $agentHash['url_contexts']);
        foreach ($contexts as $context) {
          $data['visitor_context']['querystring_context'][$context] = $context;
        }
      }
      $agent->data = $data;
      $saved = personalize_agent_save($agent);
      // Clear out any existing option sets/goals if this agent already existed.
      $option_sets = personalize_option_set_load_by_agent($agent->machine_name);
      foreach($option_sets as $option_set) {
        personalize_option_set_delete($option_set->osid);
      }
      $goals = personalize_goal_load_by_conditions(array('agent' => $agent->machine_name));
      foreach ($goals as $goal_id => $goal) {
        personalize_goal_delete($goal_id);
      }
      personalize_agent_set_status($saved->machine_name, PERSONALIZE_STATUS_NOT_STARTED);
    }
  }

  /**
   * @Given /^the "([^"]*)" personalization has the "([^"]*)" status$/
   */
  public function setCampaignStatus($agent_name, $status_name) {
    $statuses = personalize_get_agent_status_map();
    $status = array_search($status_name, $statuses);
    if ($status === FALSE) {
      throw new \Exception(sprintf('Status %s is invalid.', $status_name));
    }
    $agent = personalize_agent_load($agent_name);
    if ($agent->plugin == 'acquia_lift_target') {
      module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
      // Implement the targeting before changing the status.
      $agent_data = personalize_agent_load($agent_name);
      acquia_lift_implement_targeting($agent_data);
    }
    personalize_agent_set_status($agent_name, $status);
  }


  /**
   * @Given /^goals:$/
   *
   * Requires each row to have at least:
   * - action_name: The machine_name for the goal.
   * - agent: The campagin for the goal
   * - label: The display name for the goal if it does not a pre-existing
   *   visitor action.
   * Optional values:
   * - plugin: The type of goal, defaults to page.
   * - event: The event on for the type of goal, defaults to views.
   * - pages: The pages to limit for the goal, defaults to ''.
   * - value: The value for the goal, defaults to 1.
   */
  public function createGoals(TableNode $goalsTable) {
    foreach ($goalsTable->getHash() as $goalHash) {
      $goal = (object) $goalHash;
      $goal_value = isset($goal->value) ? $goal->value : 1;

      $actions = visitor_actions_get_actions();

      if (!isset($actions[$goal->action_name])) {
        $action = array(
          'label' => $goal->label,
          'machine_name' => $goal->action_name,
          'plugin' => isset($goal->plugin) ? $goal->plugin : 'page',
          'client_side' => 0,
          'identifier' => '',
          'event' => isset($goal->event) ? $goal->event : 'views',
          'pages' => isset($goal->pages) ? $goal->pages : '',
          'data' => array(),
          'limited_use' => 1,
        );
        visitor_actions_save_action($action);
      }
      personalize_goal_save($goal->agent, $goal->action_name, $goal_value);
    }
  }

  /**
   * @Given /^personalized elements:$/
   */
  public function createPersonalizedElements(TableNode $elementsTable) {
    foreach ($elementsTable->getHash() as $optionSetHash) {
      $option_set = (object) $optionSetHash;
      $option_set->plugin = 'elements';
      $option_set->data = array(
        'personalize_elements_selector' => $option_set->selector,
        'personalize_elements_type' => $option_set->type,
      );
      $option_set->executor = 'personalizeElements';
      $content_options = explode(',', $option_set->content);
      $options = array();
      $context_values = empty($option_set->targeting) ? array() : $this->convertContexts(explode(',', $option_set->targerting));
      foreach ($content_options as $index => $content) {
        $content = trim($content);
        $option = array(
          'option_id' => 'option-' . ($index + 1),
          'option_label' => personalize_generate_option_label($index),
          'personalize_elements_content' => $content,
        );
        // Set up fixed targeting if there's an available fixed targeting value.
        if (!empty($context_values)) {
          $option['fixed_targeting'] = array(array_shift($context_values));
        }
        $options[] = $option;
      }
      $options = personalize_ensure_unique_option_ids($options);
      $control_option = array('option_label' => PERSONALIZE_CONTROL_OPTION_LABEL, 'option_id' => PERSONALIZE_CONTROL_OPTION_ID, 'personalize_elements_content' => '');
      array_unshift($options, $control_option);
      $option_set->options = $options;
      personalize_option_set_save($option_set);
    }
  }

  /**
   * @Given /^audiences:$/
   */
  public function createAudiences(TableNode $elementsTable) {
    module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
    foreach ($elementsTable->getHash() as $audienceHash) {
      $audience = (object) $audienceHash;
      $label = $audience->label;
      $agent_name = $audience->agent;
      $context_values = empty($audience->context) ? array() : $this->convertContexts(explode(',', $audience->context));
      $weight = isset($audience->weight) ? $audience->weight : 50;
      $strategy = isset($audience->strategy) ? $audience->strategy : 'OR';
      acquia_lift_target_audience_save($label, $agent_name, $context_values, $strategy, $weight);
    }
  }

  /**
   * @Given /^targeting:$/
   */
  public function createTargeting(TableNode $targetingTable) {
    module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
    $targeting = array();
    foreach ($targetingTable->getHash() as $targetingHash) {
      $targeting[$targetingHash['agent']][$targetingHash['audience']] = explode(',', $targetingHash['options']);
    }
    foreach ($targeting as $agent => $agent_targeting) {
      $agent_data = personalize_agent_load($agent);
      acquia_lift_save_targeting_structure($agent_data, $agent_targeting);
    }
  }

  /****************************************************
   *        A S S E R T I O N S
   ***************************************************/
  /**
   * @When /^I check the "([^”]*)" radio button$/
   */
  public function iCheckTheRadioButton($radioLabel) {
    $radioButton = $this->getSession()->getPage()->findField($radioLabel);
    if (null === $radioButton) {
      throw new \Exception(sprintf('Cannot find radio button %s', $radioLabel));
    }
    $this->getSession()->getDriver()->click($radioButton->getXPath());
  }

  /**
   * @Then I should see :count for the :type count
   */
  public function assertMenuCount($count, $type) {
    switch ($type) {
      case 'variation':
      case 'variation set':
        $region_name = 'lift_tray_variation_count';
        break;
      case 'goal':
        $region_name = 'lift_tray_goal_count';
        break;
      default:
        throw new \Exception(sprintf('The count type %s is not supported.', $type));
    }
    $regions = $this->getRegions($region_name);
    foreach ($regions as $current) {
      if ($current->isVisible()) {
        $region = $current;
        break;
      }
    }
    if (empty($region)) {
      throw new \Exception(sprintf('There is no visible goal region'));
    }
    $actual_count = $region->getText();
    if ($actual_count !== $count) {
      throw new \Exception(sprintf('The count for type %s was %s rather than the expected %s.', $type, $actual_count, $count));
    }
  }

  /**
   * @When I hover over :link in the :region( region)
   *
   * @throws \Exception
   *   If region or link within it cannot be found.
   */
  public function assertRegionLinkHover($link, $region) {
    $linkObj = $this->findLinkInRegion($link, $region);
    if (empty($linkObj)) {
      throw new \Exception(sprintf('The link "%s" was not found in the region "%s" on the page %s', $link, $region, $this->getSession()->getCurrentUrl()));
    }
    $linkObj->mouseOver();
  }

  /**
   * @When I hover over :id id in the :region( region)
   *
   * @throws \Exception
   *   If region or element within it cannot be found.
   */
  public function assertRegionElementHover($id, $region) {
    $element = $this->findElementInRegion($id, $region);
    if (empty($element)) {
      throw new \Exception(sprintf('The element "%s" was not found in the region "%s" on the page %s', $id, $region, $this->getSession()->getCurrentUrl()));
    }
    $element->mouseOver();
  }

  /**
   * @When I click :selector element in the :region region
   *
   * @throws \Exception
   *   If region or element within it cannot be found.
   */
  public function assertRegionElementClick($selector, $region) {
    $element = $this->findElementInRegion($selector, $region);
    if (empty($element)) {
      throw new \Exception(sprintf('The element "%s" was not found in the region "%s" on the page %s', $selector, $region, $this->getSession()->getCurrentUrl()));
    }
    $element->click();
  }

  /**
   * @Then :variation_set set :variation variation should have the :link link
   *
   * @throws \Exception
   *   If the menu or link cannot be found.
   */
  public function assertRegionVariationHasLink($variation_set, $variation, $link) {
    $css = $this->getVariationLinkCss($variation_set, $variation, $link);
    // Now find the link and return it.
    $element = $this->findElementInRegion($css, 'lift_tray');
    if (empty($element)) {
      throw new \Exception(sprintf('Cannot load the link "%s" for set "%s" and variation "%s" on page %s using selector %s.', $link, $variation_set, $variation, $this->getSession()->getCurrentUrl(), $css));
    }
    return $element;
  }

  /**
   * @Then :variation_set set :variation variation should not have the :link link
   *
   * @throws \Exception
   *   If the menu cannot be found or the link can be found.
   */
  public function assertNotRegionVariationHasLink($variation_set, $variation, $link) {
    $css = $this->getVariationLinkCss($variation_set, $variation, $link);
    // Now find the link and return it.
    $element = $this->findElementInRegion($css, 'lift_tray');
    if (!empty($element)) {
      throw new \Exception(sprintf('Found the link "%s" for set "%s" and variation "%s" on page %s using selector %s.', $link, $variation_set, $variation, $this->getSession()->getCurrentUrl(), $css));
    }
  }

  /**
   * @Then :variation_set set :variation variation :link link is disabled
   *
   * @throws \Exception
   *   If the menu or link cannot be found.
   */
  public function assertRegionVariationHasLinkDisabled($variation_set, $variation, $link) {
    $css = $this->getVariationLinkCss($variation_set, $variation, $link);
    // Now find the link and return it.
    $element = $this->findElementInRegion($css, 'lift_tray');
    if (empty($element)) {
      throw new \Exception(sprintf('Cannot load the link "%s" for set "%s" and variation "%s" on page %s using selector %s.', $link, $variation_set, $variation, $this->getSession()->getCurrentUrl(), $css));
    }
    if (!$element->hasClass('acquia-lift-disabled')) {
      throw new \Exception(sprintf('The link "%s" for set "%s" and variation "%s" on page %s using selector %s is not disabled.', $link, $variation_set, $variation, $this->getSession()->getCurrentUrl(), $css));
    }
    return $element;
  }

  /**
   * @When I click :link link for the :variation_set set :variation variation
   *
   * @throws \Exception
   *   If the menu or link cannot be found.
   */
  public function assertRegionVariationLinkClick($link, $variation_set, $variation) {
    $element = $this->assertRegionVariationHasLink($variation_set, $variation, $link);
    if (!empty($element)) {
      $element->click();
    }
  }

  /**
   * @Then the :field field should contain text that has :needle
   *
   * @throws \Exception
   *   If the the substring cannot be found in the given field.
   */
  public function assertFieldContains($field, $needle) {
    $node = $this->assertSession()->fieldExists($field);
    $haystack = $node->getValue();
    if (strpos($haystack, $needle) === false) {
      throw new \Exception(sprintf('The field "%s" value is "%s", but we are looking for "%s".', $field, $haystack, $needle));
    }
  }

  /**
   * @Then I should see :selector element in the :region region is :state for editing
   *
   * @throws \Exception
   *   If the region or element cannot be found or is not in a specified state.
   */
  public function assertRegionElementIsInState($selector, $region, $state) {
    $state_class = array(
      'highlighted' => 'acquia-lift-element-variation-item',
      'available' => 'visitor-actions-ui-enabled',
    );
    if (!isset($state_class[$state])) {
      $state_options_array = array_keys($state_class);
      $state_options_string = implode(', ', $state_options_array);
      throw new \Exception(sprintf('The element state "%s" is not defined. Available options are "%s".', $state, $state_options_string));
    }
    $element = $this->findElementInRegion($selector, $region);
    if (empty($element)) {
      throw new \Exception(sprintf('The element "%s" was not found in the region "%s" on the page %s.', $selector, $region, $this->getSession()->getCurrentUrl()));
    }
    $class = $state_class[$state];
    if (!$element->hasClass($class)) {
      throw new \Exception(sprintf('The element "%s" in region "%s" on the page %s is not in "%s" state.', $selector, $region, $this->getSession()->getCurrentUrl(), $state));
    }
  }

  /**
   * @When I wait for Lift to synchronize
   */
  public function waitForLiftSynchronize() {
    $this->spinUntilLiftCampaignsAreSynchronized();
  }

  /**
   * @Then I should visibly see the link :link in the :region( region)
   *
   * @throws \Exception
   *   If region or link within it cannot be found or is hidden.
   */
  public function assertLinkVisibleRegion($link, $region) {
    $results = $this->findLinksInRegion($link, $region);
    if (empty($results)) {
      throw new \Exception(sprintf('No link to "%s" in the "%s" region on the page %s', $link, $region, $this->getSession()->getCurrentUrl()));
    }
    foreach ($results as $result) {
      if ($result->isVisible()) {
        return;
      }
    }
    throw new \Exception(sprintf('No link to "%s" is visible in the "%s" region on the page %s', $link, $region, $this->getSession()->getCurrentUrl()));
  }

  /**
   * @Then I should not visibly see the link :link in the :region( region)
   *
   * @throws \Exception
   *   If link is found in region and is visible.
   */
  public function assertNotLinkVisibleRegion($link, $region) {
    $result = $this->findLinksInRegion($link, $region);
    if (empty($results)) {
      return;
    }
    foreach($result as $element) {
      if ($element->isVisible()) {
        throw new \Exception(sprintf('Link to "%s" in the "%s" region on the page %s', $link, $region, $this->getSession()->getCurrentUrl()));
      }
    }
  }

  /**
   * @Then /^I should see the modal with title "([^"]*)"$/
   *
   * @throws \Exception
   *   If the modal with the specified title cannot be found on the page or is
   *   invisible.
   */
  public function assertModalWindowWithTitle($title) {
    $region = $this->getRegion('modal_title');
    if (!$region || !$region->isVisible()) {
      throw new \Exception(sprintf('The modal dialog titled %s is not visible on the page %s', $title, $this->getSession()->getCurrentUrl()));
    }
    $regionText = $region->getText();
    if (strpos($regionText, $title) === FALSE) {
      throw new \Exception(sprintf("The title '%s' was not found in the modal title region on the page %s", $title, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then /^I should not see the modal$/
   */
  public function assertNoModalWindow() {
    $this->assertNoRegion('modal_content', 'modal dialog');
  }

  /**
   * @Then /^I should not see the variation type dialog$/
   */
  public function assertNoVariationTypeDialogWindow() {
    $this->assertNoRegion('dialog_variation_type', 'variation type dialog');
  }

  /**
   * @Then /^I should not see the variation type form dialog$/
   */
  public function assertNoVariationTypeFormDialogWindow() {
    $this->assertNoRegion('dialog_variation_type_form', 'variation type form dialog');
  }

  /**
   * @Given /^menu item "([^"]*)" should be "(active|inactive)"$/
   */
  public function assertMenuItemInactive($link, $status) {
    $elements = $this->findLinksInRegion($link, 'lift_tray');
    if (empty($elements)) {
      throw new \Exception(sprintf('The link element %s was not found on the page %s', $link, $this->getSession()->getCurrentUrl()));
    }
    $found = FALSE;
    foreach ($elements as $element) {
      /**
       * This logis is not ideal.  It would be better to actually only check each
       * visible item and then report directly based on whether the item was
       * inactive or active, however, there is a selenium bug that is treating
       * both "Add variation set" links and "Add goal" links as invisible even
       * when one is shown on the screen.
       */
      $found = TRUE;
      if ($element->hasClass('acquia-lift-menu-disabled') || $element->hasClass('acquia-lift-disabled')) {
        if ($status === 'inactive') {
          $found = TRUE;
          continue;
        }
      }
      else if ($status === 'active') {
        $found = TRUE;
        continue;
      }
    }
    if (!$found) {
      throw new \Exception(sprintf('The link element %s was not %s on the page %s', $link, $status, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then I should see the visitor action edit link for the :action action
   */
  public function assertVisitorActionEditLink($action_name) {
    $element = $this->getVisitorActionEditLink($action_name);
    if (empty($element)) {
      throw new \Exception(sprintf('The edit link for %s action was not found in the %s region on page %s', $action_name, 'campaign_workflow_form', $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then I should not see the visitor action edit link for the :action action
   */
  public function assertNoVisitorActionEditLink($action_name) {
    $element = $this->getVisitorActionEditLink($action_name);
    if (!empty($element)) {
      throw new \Exception(sprintf('The edit link for %s action was found in the %s region on page %s', $action_name, 'campaign_workflow_form', $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @When I click the visitor action edit link for the :action action
   */
  public function assertVisitorActionEditLinkClick($action_name) {
    $element = $this->getVisitorActionEditLink($action_name);
    if (empty($element)) {
      throw new \Exception(sprintf('The edit link for %s action was not found in the %s region on page %s', $action_name, 'campaign_workflow_form', $this->getSession()->getCurrentUrl()));
    }
    $element->click();
  }

  /**
   * @Then I should see element with :id id in :region region with the :class class
   */
  public function assertElementWithIDHasClass($id, $region, $class) {
    $element = $this->findElementInRegion($id, $region);
    if (empty($element)) {
      throw new \Exception(sprintf('The element with %s was not found in region %s on the page %s', $id, $region, $this->getSession()->getCurrentUrl()));
    }
    if (!$element->hasClass($class)) {
      throw new \Exception(sprintf('The element with id %s in region %s on page %s does not have class %s', $id, $region, $this->getSession()->getCurrentUrl(), $class));
    }
  }

  /**
   * @Then I should see the message :text in the messagebox
   */
  public function assertTextInMessagebox($text) {
    $this->spinUntilMessageBoxIsPopulated();

    $script = "return jQuery('#acquia-lift-message-box').find('.message').text();";
    $message = $this->getSession()->evaluateScript($script);
    if (strpos($message, $text) === FALSE) {
      throw new \Exception(sprintf('The message "%s" was not found in the messagebox.', $text));
    }
  }

  /**
   * @When I move the :variation variation to the :audience audience
   */
  public function assignVariationFromUnassigned($variation, $audience) {
    $variation_element = $this->getAssignableVariation($variation);
    $to_audience_element = $this->getAudienceElement($audience);
    if (empty($variation_element)) {
      throw new \Exception(sprintf('Cannot find variation "%s" in available options.', $variation));
    }
    if (empty($to_audience_element)) {
      throw new \Exception(sprintf('Cannot find audience "%s" to move variation to.', $audience));
    }
    $to_audience_drop_zone = $to_audience_element->find('css', '.acquia-lift-targeting-droppable');
    if (empty($to_audience_drop_zone)) {
      throw new \Exception(sprintf('Cannot find drop zone for audience "%s".', $audience));
    }
    $variation_element->dragTo($to_audience_drop_zone);
  }

  /**
   * @When I :action the :variation variation from the :from_audience audience to the :to_audience audience
   */
  public function assignVariationToAudience($action, $variation, $from_audience, $to_audience) {
    $valid_actions = array('move', 'copy');
    if (!in_array($action, $valid_actions)) {
      throw new \Exception(sprintf('Invalid action "%s" supplied for variation assignment.  Valid actions are: %s.', $action, implode(', ', $valid_actions)));
    }
    $from_audience_element = $this->getAudienceElement($from_audience);
    $to_audience_element = $this->getAudienceElement($to_audience);
    if (empty($from_audience_element)) {
      throw new \Exception(sprintf('The "%s" audience cannot be found on the current page.', $from_audience));
    }
    if (empty($to_audience_element)) {
      throw new \Exception(sprintf('The "%s" audience cannot be found on the current page.', $to_audience));
    }
    $from_variations_list = $from_audience_element->findAll('css', '.acquia-lift-draggable-variations li');
    foreach ($from_variations_list as $variation_element) {
      $variation_list_item = $variation_element->getText();
      if (strpos($variation_list_item, $variation) === 0) {
        $variation_draggable = $action == 'move' ? $variation_element : $variation_element->find('css', '.acquia-lift-targeting-duplicate-icon');
        break;
      }
    }
    if (empty($variation_draggable)) {
      throw new \Exception(sprintf('The "%s" variation cannot be found in the list of variations for "%s" audience as a draggable option.', $variation, $from_audience));
    }
    $to_audience_drop_zone = $to_audience_element->find('css', '.acquia-lift-targeting-droppable');
    $variation_draggable->dragTo($to_audience_drop_zone);
  }

  /**
   * @Then :variation variation should be assigned to the :audience audience
   */
  public function assertAudienceHasVariation($variation_label, $audience_label) {
    $escaped_variation = $this->getSession()->getSelectorsHandler()->xpathLiteral($variation_label);
    $audience_element = $this->getAudienceElement($audience_label);
    if (empty($audience_element)) {
      throw new \Exception(sprintf('The "%s" audience container cannot be found on the page.', $audience_label));
    }
    $variation_select = $audience_element->find('css', 'select[name*=assignment]');
    $variation_option = $variation_select->find('named', array('option', $escaped_variation));
    if (empty($variation_option)) {
      throw new \Exception(sprintf('The "%s" variation cannot be found in the "%s" audience variation select options.', $variation_label, $audience_label));
    }
    if (!$variation_option->isSelected()) {
      throw new \Exception(sprintf('The "%s" variation is not selected for the "%s" audience.', $variation_label, $audience_label));
    }
    $variations_list = $audience_element->find('css', '.acquia-lift-draggable-variations');
    $variation_display = $variations_list->find('named', array('content', $escaped_variation));
    if (empty($variation_display)) {
      throw new \Exception(sprintf('The "%s" variation is not displayed for "%s" audience.', $variation_label, $audience_label));
    }
  }

  /**
   * @Then :variation variation should not be assigned to the :audience audience
   */
  public function assertAudienceHasNoVariation($variation_label, $audience_label) {
    $escaped_variation = $this->getSession()->getSelectorsHandler()->xpathLiteral($variation_label);
    $audience_element = $this->getAudienceElement($audience_label);
    if (empty($audience_element)) {
      throw new \Exception(sprintf('The "%s" audience container cannot be found on the page.', $audience_label));
    }
    $variation_select = $audience_element->find('css', 'select[name*=assignment]');
    $variation_option = $variation_select->find('named', array('option', $escaped_variation));
    if (empty($variation_option)) {
      throw new \Exception(sprintf('The "%s" variation cannot be found in the "%s" audience variation select options.', $variation_label, $audience_label));
    }
    if ($variation_option->isSelected()) {
      throw new \Exception(sprintf('The "%s" variation is selected for the "%s" audience.', $variation_label, $audience_label));
    }
    $variations_list = $audience_element->find('css', '.acquia-lift-draggable-variations');
    $variation_display = $variations_list->find('named', array('content', $escaped_variation));
    if (!empty($variation_display)) {
      throw new \Exception(sprintf('The "%s" variation is displayed for "%s" audience.', $variation_label, $audience_label));
    }
  }

  /****************************************************
   *        H E L P E R  F U N C T I O N S
   ***************************************************/

  /**
   * Helper function to get the visitor action edit link for a particular action
   *
   * @param string $action_name
   *   The machine name for the visitor action
   * @return \Behat\Mink\Element\NodeElement|null
   *   The element node for the link or null if not found.
   */
  public function getVisitorActionEditLink($action_name) {
    $id = '#edit-visitor-action-' . $action_name;
    return $this->findElementInRegion($id, 'campaign_workflow_form');
  }

  /**
   * Helper function to generate the css for a variation action link.
   *
   * @param string $variation_set
   *   The name of the variation set displayed
   * @param string $variation
   *   The name of the variation displayed
   * @param string $link
   *   The link text to find.  One of "rename", "edit", or "delete".
   *
   * @throws \Exception
   *   If the link type is invalid or the current campaign is not available.
   */
  public function getVariationLinkCss($variation_set, $variation, $link) {
    $link = drupal_strtolower($link);
    if (!in_array($link, array('edit', 'rename', 'delete'))) {
      throw new \Exception(sprintf('The variation action "%s" is invalid.', $link));
    }
    $campaign = $this->getCurrentCampaign();
    if (empty($campaign)) {
      throw new \Exception(sprintf('Cannot determine the current personalization for variation set %s.', $variation_set));
    }
    $agent_instance = personalize_agent_load_agent($campaign);
    if (empty($agent_instance)) {
      throw new \Exception(sprintf('Cannot load the current agent instance for personalization %s.', $campaign));
    }
    $option_sets = personalize_option_set_load_by_agent($campaign, TRUE);
    foreach ($option_sets as $option_set) {
      if ($option_set->label == $variation_set) {
        $osid = $option_set->osid;
        foreach ($option_set->options as $option) {
          if ($option['option_label'] == $variation) {
            $option_id = $option['option_id'];
            break;
          }
        }
        break;
      }
    }
    if (empty($osid)) {
      throw new \Exception(sprintf('Cannot load the option set %s for personalization %s.', $variation_set, $campaign));
    }
    $css = '.acquia-lift-menu-item[data-acquia-lift-personalize-option-set="' . personalize_stringify_osid($osid) . '"]';
    switch ($link) {
      case "edit":
        $css .= ' a.acquia-lift-variation-edit';
        break;
      case "rename":
        $css .= ' a.acquia-lift-variation-rename';
        break;
      case "delete":
        $css .= ' a.acquia-lift-variation-delete';
        break;
    }
    $css .= '[data-acquia-lift-personalize-option-set-option="' . $option_id . '"]';

    return $css;
  }

  /**
   * Helper function to retrieve a context parameter.
   *
   * @param $param_name
   *   The name of the parameter to retrieve
   * @return
   *   The parameter value or NULL if undefined.
   */
  public function getContextParameter($param_name) {
    return !empty($this->context_parameters[$param_name]) ? $this->context_parameters[$param_name] : NULL;
  }

  /**
   * Helper function to set the context parameters.
   *
   * @param array $parameters
   *   The parameters to set.
   */
  public function setContextParameters($parameters) {
    $this->context_parameters = array_merge($this->context_parameters, $parameters);
  }

  /**
   * Helper function to return a link in a particular region.
   *
   * Note: the selector is translated to xpath in order to allow selection of
   * the link even if it needs to be scrolled in order to visible.
   *
   * @param string $link
   *   link id, title, text or image alt
   * @param $region
   *   region identifier from configuration.
   *
   * @return \Behat\Mink\Element\NodeElement|null
   *   The element node for the link or null if not found.
   */
  private function findLinkInRegion($link, $region) {
    $regionObj = $this->getRegion($region);
    $element = $regionObj->findLink($link);
    return $element;
  }

  /**
   * Helper function to return any link in a particular region marching the
   * link by id, title, text, or alt.
   *
   * @param string $link
   *   link id, title, text or image alt
   * @param $region
   *   region identifier from configuration.
   *
   * @return array|null
   *   An array of  \Behat\Mink\Element\NodeElement objects.
   */
  private function findLinksInRegion($link, $region) {
    $regionObj = $this->getRegion($region);
    $session = $this->getSession();
    $escapedValue = $session->getSelectorsHandler()->xpathLiteral($link);
    $elements = $regionObj->findAll('named', array('link', $escapedValue));
    return $elements;
  }

  /**
   * Helper function to return an element in a particular region.
   *
   * @param string $selector
   *   the css selector for an element
   * @param $region
   *   region identifier from configuration.
   *
   * @return \Behat\Mink\Element\NodeElement|null
   *   The element node for the link or null if not found.
   */
  private function findElementInRegion($selector, $region) {
    $regionObj = $this->getRegion($region);
    return $regionObj->find('css', $selector);
  }

  /**
   * Thin wrapper over Drupal MinkContext's getRegion function.
   *
   * @param $region
   *   The region identifier to load.
   *
   * @return \Behat\Mink\Element\NodeElement|null
   *   The region element node or null if not found.
   *
   * @throws \Exception
   *   If the region cannot be found on the current page.
   */
  private function getRegion($region) {
    return $this->contexts['Drupal\DrupalExtension\Context\MinkContext']->getRegion($region);
  }

  /**
   * Helper function to retrieve a region defined in the configuration file that
   * may consist of multiple elements matching the selector.
   *
   * @param $region
   *   The region identifier to load.
   *
   * @return \Behat\Mink\Element\NodeElement|null
   *   The region element node or null if not found.
   *
   * @throws \Exception
   *   If the region cannot be found on the current page.
   */
  private function getRegions($region) {
    $mink = $this->getMink();
    $regions = $mink->getSession()->getPage()->findAll('region', $region);
    if (empty($regions)) {
      throw new \Exception(sprintf('The region %s was not found on the page %s', $region, $this->getSession()->getCurrentUrl()));
    }
    return $regions;
  }

  /**
   * Helper function to assert that a particular region is not visible.
   *
   * @param $region_id
   *   The id for the region defined in the behat.yml configuration file.
   * @param $region_name
   *   A human readable name for the region
   * @throws Exception
   *   If the region is visible on the page.
   */
  public function assertNoRegion($region_id, $region_name) {
    try {
      $region = $this->getRegion($region_id);
    } catch (\Exception $e) {
      // If the region was not found that is good.
      return;
    }
    if ($region && $region->isVisible()) {
      throw new \Exception(sprintf('The %s was found on the page %s', strtolower($region_name), $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * Helper function to retrieve the currently active campaign from the client
   * Javascript.
   *
   * @return string
   *   The machine name for the currently active campaign or empty string.
   */
  private function getCurrentCampaign() {
    $script = 'return Drupal.settings.personalize.activeCampaign;';
    return $this->getMink()->getSession()->evaluateScript($script);
  }

  /**
   * Helper function to retrieve a specific audience targeting element for the
   * current campaign.
   *
   * @param $audience_label
   *   The audience label assigned.
   * @return NodeElement|null
   *   The element found for the audience container or null if not found.
   * @throws Exception
   *   If the audience is not part of the current campaign.
   */
  private function getAudienceElement($audience_label) {
    $agent_name = $this->getCurrentCampaign();
    module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
    $option_set = acquia_lift_get_option_set_for_targeting($agent_name);
    if (!isset($option_set->targeting)) {
      throw new \Exception(sprintf('The current agent "%s" does not have any audiences available.', $agent_name));
    }
    $audiences = array();
    foreach ($option_set->targeting as $audience_id => $audience) {
      $audiences[$audience['label']] = $audience_id;
    }
    if (!isset($audiences[$audience_label])) {
      throw new \Exception(sprintf('The current agent "%s" does not have an audience named "%s".', $agent_name, $audience_label));
    }
    return $this->findElementInRegion('#edit-audiences-existing-' . $audiences[$audience_label], 'campaign_workflow_form');
  }

  /**
   * Helper function to retrieve a specific variation from the available
   * variations list.
   *
   * @param $variation
   *   The label of the variation
   * @throws Exception
   *   If the variation is not available within the available variations.
   */
  private function getAssignableVariation($variation) {
    $variations_list = $this->findElementInRegion('.form-item-variations-options-assignment', 'campaign_workflow_form');
    if (empty($variations_list)) {
      throw new \Exception('Could not find the variations bucket area for all variations.');
    }
    $variation_items = $variations_list->findAll('css', '.acquia-lift-draggable-variations li');
    foreach ($variation_items as $variation_element) {
      $variation_list_item = $variation_element->getText();
      if (strpos($variation_list_item, $variation) === 0) {
        return $variation_element;
      }
    }
    return NULL;
  }

  /**
   * Generates the targeting contexts based on a list of context values
   * @param array $feature_contexts
   *   Feature contexts to be added
   * @return array
   *   The context values formatted for Lift.
   */
  private function convertContexts($feature_contexts) {
    $context_values = array();
    // Grab explicit targeting values if specified.
    if (!empty($values)) {
      $contexts = variable_get('personalize_url_querystring_contexts', array());
      foreach ($feature_contexts as $context) {
        if (isset($contexts[$context])) {
          foreach ($contexts[$context] as $value) {
            $context_values[] = $context . '::' . $value;
          }
        }
      }
    }
    return $context_values;
  }

  /****************************************************
   *        S P I N  F U N C T I O N S
   ***************************************************/
  /**
   * Keep retrying assertion for a defined number of iterations.
   *
   * @param closure $lambda           Callback.
   * @param integer $attemptThreshold Number of attempts to execute the command.
   *
   * @throws \Exception If attemptThreshold is met.
   *
   * @return mixed
   */
  private function spin($lambda, $attemptThreshold = 15) {
    for ($iteration = 0; $iteration <= $attemptThreshold; $iteration++) {
      try {
        if (call_user_func($lambda)) {
          return;
        }
      } catch (\Exception $exception) {
        // do nothing
      }

      sleep(1);
    }
  }

  /**
   * Spin JavaScript evaluation.
   *
   * @param string  $assertionScript  Assertion script
   * @param integer $attemptThreshold Number of attempts to execute the command.
   */
  private function spinJavaScriptEvaluation($assertionScript, $attemptThreshold = 15) {
    $this->spin(function () use ($assertionScript) {
      return $this->getMink()->getSession()->evaluateScript('return ' . $assertionScript);
    }, $attemptThreshold);
  }

  /**
   * Spin until the Ajax is finished.
   */
  private function spinUntilAjaxIsFinished() {
    $assertionScript = '(typeof(jQuery)=="undefined" || (0 === jQuery.active && 0 === jQuery(\':animated\').length));';
    $this->spinJavaScriptEvaluation($assertionScript);
  }

  /**
   * Spin until the message box is populated.
   */
  private function spinUntilMessageBoxIsPopulated() {
    $assertionScript = "(jQuery('#acquia-lift-message-box').length > 0 && jQuery('#acquia-lift-message-box').hasClass('acquia-lift-messagebox-shown'));";
    $this->spinJavaScriptEvaluation($assertionScript);
  }

  /**
   * Spin until the Lift Campaigns are synchronized.
   */
  private function spinUntilLiftCampaignsAreSynchronized() {
    $assertionScript = '(typeof(jQuery)=="undefined" || (0 === jQuery.active && 0 === Drupal.acquiaLift.queueCount));';
    $this->spinJavaScriptEvaluation($assertionScript);
  }
}
