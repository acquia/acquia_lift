# Assumes roles: Marketer

Feature: Creation of Simple A/B campaigns from unified navigation.
  In order to quickly create A/B tests
  As a site marketer
  I want the ability to create a campaign in context.

@api @javascript @campaign
Scenario: Create the simplest A/B campaign from start to finish.
  Given I am logged in as a user with the "Marketer" role
  And I am on the homepage
  When I click "Acquia Lift" in the "menu" region
  And I wait for AJAX to finish
  Then I should see the link "Campaigns" in the "lift_tray" region
  And I should not see the link "Variations" visible in the "lift_tray" region
  And I should not see the link "Variation sets" visible in the "lift_tray" region
  And I should not see the link "Goals" visible in the "lift_tray" region
  And I should not see the link "Reports" visible in the "lift_tray" region
  And I should not see the link "Status" visible in the "lift_tray" region
  When I hover over "Campaigns" in the "lift_tray" region
  And I wait for AJAX to finish
  Then I should see the link "Add campaign" in the "lift_tray" region
  When I click "Add campaign" in the "lift_tray" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Create a campaign"
  And I should see the link "A/B test" in the "modal_content" region
  And I should see the link "Custom Lift campaign" in the "modal_content" region
  When I click "A/B test" in the "modal_content" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Create a campaign"
  And I should see the link "Change type of test" in the "modal_content" region
  When I click "Change type of test" in the "modal_content" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Create a campaign"
  And I should see the link "A/B test" in the "modal_content" region
  And I should see the link "Custom Lift campaign" in the "modal_content" region
  When I click "A/B test" in the "modal_content" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Create a campaign"
  And I should see the link "Change type of test" in the "modal_content" region
  When I fill in "My test campaign" for "edit-agent-basic-info-title"
  And I press the "Create campaign" button
  And I wait for AJAX to finish
  Then I should see the message "Click the element you want to change in Variation #1" in the messagebox
  When I wait for AJAX to finish
  Then I should see the text "Campaign: My test campaign" in the "lift_tray_campaign_header" region
  And I should see "0" for the "variation" count
  And I should see the link "Variations" visible in the "lift_tray" region
  And I should see the link "Goals" visible in the "lift_tray" region
  And I should see "0" for the "goal" count
  #And menu item "Reports" should be "inactive"
  And menu item "Start campaign" should be "inactive"
  And I should not see the modal
  And the variation edit mode is "active"
  When I click "#site-name a span" element in the "page_content" region
  And I wait for AJAX to finish
  Then I should see the text "<SPAN>" in the "dialog_variation_type" region
  When I click "Edit text" in the "dialog_variation_type" region
  And I wait for AJAX to finish
  Then I should not see a "#acquia-lift-modal-variation-type-select-dialog" element
  And I should see the text "Edit text: <SPAN>" in the "dialog_variation_type_form" region
  And the "personalize_elements_content" field should contain the site title
  When I fill in "Lift Testing" for "personalize_elements_content"
  And I press the "Save" button
  And I wait for AJAX to finish
  Then I should see the message "The variation has been created. Add one or more goals by clicking Goals > Add goal." in the messagebox
  When I wait for AJAX to finish
  Then I should not see the variation type form dialog
  And I should see the text "Lift Testing"
  And the variation edit mode is "inactive"
  When I hover over "Goals" in the "lift_tray" region
  Then I should see the link "Add goal" in the "lift_tray" region
  When I click "Add goal" in the "lift_tray" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Add a goal"
  And I should see the link "Predefined goal" in the "modal_content" region
  And I should see the link "New element goal" in the "modal_content" region
  And I should see the link "New page goal" in the "modal_content" region
  When I click "Predefined goal" in the "modal_content" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Add a goal"
  And I should see the link "Change type of goal" in the "modal_content" region
  When I click "Change type of goal" in the "modal_content" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Add a goal"
  And I should see the link "Predefined goal" in the "modal_content" region
  And I should see the link "New element goal" in the "modal_content" region
  And I should see the link "New page goal" in the "modal_content" region
  When I click "New page goal" in the "modal_content" region
  And I wait for AJAX to finish
  Then I should see the modal with title "Add a goal"
  And I should see the link "Change type of goal" in the "modal_content" region
  When I fill in "My test goal" for "edit-title"
  And I press "Add goal"
  And I wait for AJAX to finish
  Then I should see the message "My test goal goal added to campaign." in the messagebox
  When I wait for AJAX to finish
  Then  I should see "1" for the "goal" count
  When I hover over "Goals" in the "lift_tray" region
  Then I should see the text "My test goal" in the "lift_tray" region
  When I wait for Lift to synchronize
  Then menu item "Reports" should be "active"
  And menu item "Start campaign" should be "active"

  @api @javascript @campaign
  Scenario:  As a Marketer I want to quickly create several A/B test campaigns for later use.
    Given I am logged in as a user with the "Marketer" role
    And I am on the homepage
    When I click "Acquia Lift" in the "menu" region
    And I wait for AJAX to finish
    Then I should see the link "Campaigns" in the "lift_tray" region
    When I hover over "Campaigns" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the link "Add campaign" in the "lift_tray" region
    When I click "Add campaign" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the modal with title "Create a campaign"
    When I click "A/B test" in the "modal_content" region
    And I wait for AJAX to finish
    Then I should see the modal with title "Create a campaign"
    When I fill in "Test campaign 2" for "agent_basic_info[title]"
    And I press the "Create campaign" button
    And I wait for AJAX to finish
    Then I should see the message "Click the element you want to change in Variation #1" in the messagebox
    When I wait for AJAX to finish
    Then I should see the text "Campaign: Test campaign 2" in the "lift_tray_campaign_header" region
    And the variation edit mode is "active"
    When I hover over "Campaign: Test campaign 2" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the link "Add campaign" in the "lift_tray" region
    When I click "Add campaign" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the modal with title "Create a campaign"
    And the variation edit mode is "disabled"
    When I click "A/B test" in the "modal_content" region
    And I wait for AJAX to finish
    Then I should see the modal with title "Create a campaign"
    When I fill in "Test campaign 3" for "agent_basic_info[title]"
    And I press the "Create campaign" button
    And I wait for AJAX to finish
    Then I should see the message "Click the element you want to change in Variation #1" in the messagebox
    When I wait for AJAX to finish
    Then I should see the text "Campaign: Test campaign 3" in the "lift_tray_campaign_header" region
    And the variation edit mode is "active"
