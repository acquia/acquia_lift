#Assumes Marketer role.
Feature: Personalize elements variations can be added to an existing campaign.
  In order to create element variations in context
  As a site marketer
  I want the ability to add personalize element variations to existing campaigns.

  @api @javascript @campaign
  Scenario: Add personalize elements to a campaign
    Given "acquia_lift" agents:
      | machine_name                    | label |
      | testing-campaign-add-variations | Testing campaign add variations |
    And I am logged in as a user with the "Marketer" role
    And I am on the homepage
    When I click "Acquia Lift" in the "menu" region
    And I wait for AJAX to finish
    Then I should see the link "Campaigns" in the "lift_tray" region
    When I hover over "Campaigns" in the "lift_tray" region
    And I click "Testing campaign add variations" in the "lift_tray" region
    Then I should see the link "Variation Sets" visible in the "lift_tray" region
    And I should see the text "0" in the "lift_tray_variation_count" region
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the link "Add variation set" in the "lift_tray" region
    When I click "Add variation set"
    And I wait for AJAX to finish
    Then I should see the modal with title "Add a variation set"
    And I should see the link "Webpage elements" in the "modal_content" region
    And I should see the link "Drupal blocks" in the "modal_content" region
    When I click "Webpage elements" in the "modal_content" region
    And I wait for AJAX to finish
    Then I should not see the modal
    When I click "#site-name a span" element in the "page_content" region
    And I wait for AJAX to finish
    Then I should see the text "<SPAN>" in the "dialog_variation_type" region
    When I click "Edit text" in the "dialog_variation_type" region
    And I wait for AJAX to finish
    Then I should not see the variation type dialog
    And I should see the text "Edit text: <SPAN>" in the "dialog_variation_type_form" region
    And the "personalize_elements_content" field should contain "lift.local"
    When I fill in "Test 1" for "personalize_elements_content"
    And I fill in "Test variation set" for "title"
    And I press the "Save" button
    And I wait for AJAX to finish
    Then I should see the message "The variation set has been created." in the messagebox
    When I wait for AJAX to finish
    Then I should not see the variation type form dialog
    And I should see the text "Test 1" in the "page_content" region
    And I should see the text "1" in the "lift_tray_variation_count" region
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the text "Test variation set" in the "lift_tray" region
    And I should see the link "Control variation" in the "lift_tray" region
    And I should see the link "Variation #1" in the "lift_tray" region
    And I should see the link "Add variation" in the "lift_tray" region
    And I should see the link "Add variation set" in the "lift_tray" region
    When I click "Add variation" in the "lift_tray" region
    Then "#site-name a span" element in the "page_content" region should have "acquia-lift-page-variation-item" class
    And I should see the text "Edit text: <SPAN>" in the "dialog_variation_type_form" region
    And I should not see the link "Edit selector" visible in the "dialog_variation_type_form" region
    When I fill in "Test 2" for "personalize_elements_content"
    And I press the "Save" button
    And I wait for AJAX to finish
    Then I should see the message "The variation has been created." in the messagebox
    When I wait for AJAX to finish
    Then I should not see the variation type form dialog
    And I should see the text "Test 2" in the "page_content" region
    And I should see the text "1" in the "lift_tray_variation_count" region
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the link "Variation #2" in the "lift_tray" region
