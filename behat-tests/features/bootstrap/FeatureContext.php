<?php

use Drupal\DrupalExtension\Context\RawDrupalContext;
use Behat\Behat\Context\SnippetAcceptingContext;
use Behat\Gherkin\Node\TableNode;
use Behat\Behat\Hook\Scope\AfterStepScope;
use Behat\Mink\Driver\Selenium2Driver;

/**
 * Defines application features from the specific context.
 */
class FeatureContext extends RawDrupalContext implements SnippetAcceptingContext {

  /**
   * Stores the context parameters that are passed in for the test suite.
   * Parameters include default values for:
   *   - temp_path: The path to temporary location where files, such as error
   *     screenshots, can be written.  Default value: /tmp/behat
   */
  protected $context_parameters = array(
    'temp_path' => '/tmp/behat',
  );

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
   * Gets a reference to current campaigns, option sets, goals, etc. for
   * tracking purposes.
   *
   * @BeforeScenario @campaign
   */
  public function before($event) {
    $this->campaigns = personalize_agent_load_multiple();
  }

  /**
   * Delete any campaigns, option sets, goals, etc. created during the
   * scenario.
   *
   * @AfterScenario @campaign
   */
  public function after($event) {
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
      echo "Saved Screenshot To $fileame \n";
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
      personalize_agent_set_status($saved->machine_name, PERSONALIZE_STATUS_RUNNING);
    }
  }

  /****************************************************
   *        A S S E R T I O N S
   ***************************************************/

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
   * @Then :selector element in the :region region should have :class class
   *
   * @throws \Exception
   *   If the region or element cannot be found or does not have the specified
   *   class.
   */
  public function assertRegionElementHasClass($selector, $region, $class) {
    $element = $this->findElementInRegion($selector, $region);
    if (empty($element)) {
      throw new \Exception(sprintf('The element "%s" was not found in the region "%s" on the page %s', $selector, $region, $this->getSession()->getCurrentUrl()));
    }
    if (!$this->elementHasClass($element, $class)) {
      throw new \Exception(sprintf('The element "%s" in region "%s" on the page %s does not have class "%s".', $selector, $region, $this->getSession()->getCurrentUrl(), $class));
    }
  }

  /**
   * @When I wait for messagebox to close
   */
  public function waitForMessageboxClose() {
    $region = $this->getRegion('messagebox');
    if (empty($region)) {
      return;
    }
    $this->getSession()->wait(5000, 'jQuery("#acquia-lift-message-box").hasClass("element-hidden")');
  }

  /**
   * @When I wait for Lift to synchronize
   */
  public function waitForLiftSynchronize() {
    $this->getSession()->wait(5000, '(typeof(jQuery)=="undefined" || (0 === jQuery.active && 0 === Drupal.acquiaLift.queueCount))');
  }

  /**
   * @Then I should see the link :link visible in the :region( region)
   *
   * @throws \Exception
   *   If region or link within it cannot be found or is hidden.
   */
  public function assertLinkVisibleRegion($link, $region) {
    $result = $this->findLinkInRegion($link, $region);
    if (empty($result) || !$result->isVisible()) {
      throw new \Exception(sprintf('No link to "%s" in the "%s" region on the page %s', $link, $region, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then I should not see the link :link visible in the :region( region)
   *
   * @throws \Exception
   *   If link is found in region and is visible.
   */
  public function assertNotLinkVisibleRegion($link, $region) {
    $result = $this->findLinkInRegion($link, $region);
    if (!empty($result) && $result->isVisible()) {
      throw new \Exception(sprintf('Link to "%s" in the "%s" region on the page %s', $link, $region, $this->getSession()->getCurrentUrl()));
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
    $class = 'acquia-lift-menu-disabled';
    $element = $this->findLinkInRegion($link, 'lift_tray');
    if (empty($element)) {
      throw new \Exception(sprintf('The link element %s was not found on the page %s', $link, $this->getSession()->getCurrentUrl()));
    }
    if ($this->elementHasClass($element, $class)) {
      if ($status === 'active') {
        throw new \Exception(sprintf('The link element %s on page %s is inactive but should be active.', $link, $this->getSession()->getCurrentUrl()));
      }
    }
    else if ($status === 'inactive') {
      throw new \Exception(sprintf('The link element %s on page %s is active but should be inactive.', $link, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then I should see element with :id id in :region region with the :class class
   */
  public function assertElementWithIDHasClass($id, $region, $class) {
    $element = $this->findElementInRegion($id, $region);
    if (empty($element)) {
      throw new \Exception(sprintf('The element with %s was not found in region %s on the page %s', $id, $region, $this->getSession()->getCurrentUrl()));
    }
    if (!$this->elementHasClass($element, $class)) {
      throw new \Exception(sprintf('The element with id %s in region %s on page %s does not have class %s', $id, $region, $this->getSession()->getCurrentUrl(), $class));
    }
  }

  /**
   * @Then I should see the message :text in the messagebox
   */
  public function assertTextInMessagebox($text) {
    // Wait for the message box to be shown.
    $this->getSession()->wait(5000, "(jQuery('#acquia-lift-message-box').length > 0 && jQuery('#acquia-lift-message-box').hasClass('acquia-lift-messagebox-shown'))");
    $script = "return jQuery('#acquia-lift-message-box').find('.message').text();";
    $message = $this->getSession()->evaluateScript($script);
    if (strpos($message, $text) === FALSE) {
      throw new \Exception(sprintf('The message "%s" was not found in the messagebox.', $text));
    }
  }

  /****************************************************
   *        H E L P E R  F U N C T I O N S
   ***************************************************/

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
    return $regionObj->findLink($link);
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
   * Helper function to retrieve a region defined in the configuration file
   * from the browser output.
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
    $mink = $this->getMink();
    $regionObj = $mink->getSession()->getPage()->find('region', $region);
    if (empty($regionObj)) {
      throw new \Exception(sprintf('The region %s was not found on the page %s', $region, $this->getSession()->getCurrentUrl()));
    }
    return $regionObj;
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
   * Helper function to determine an element has a particular class applied.
   *
   * @param \Behat\Mink\Element\NodeElement $element
   *   The element to test.
   * @param string $class
   *   The class to find.
   */
  private function elementHasClass($element, $class) {
    $classes = $element->getAttribute('class');
    $search_classes = explode(' ', $classes);
    return in_array($class, $search_classes);
  }
}
