@api @javascript @insulated @campaign
Feature: Personalizations can be selected within the unibar and the appropriate
  menus update.
  In order to manage personalizations in context and to preview variations
  As a site marketer
  I want the ability to select and preview personalizations in context.

  Scenario: Select an editable personalization.
    # I have a campaign with a variation set and a goal.
    # I login with the marketer role.
    # I am on an article page.
    Given "acquia_lift_target" agents:
      | machine_name              | label                     |
      | testing-campaign-editable | Testing campaign editable |
    And personalized elements:
      | label        | agent                     | selector    | type     | content                                    |
      | Muppet mania | testing-campaign-editable | #page-title | editText | The Rainbow Connection, Moving Right Along |
    And I am logged in as a user with the "access administration pages,access toolbar,administer visitor actions,manage personalized content" permission
    And I am viewing an "Article" content:
      | title | Test Article Title - Original |

    # I select the campaign.
    When I click "Acquia Lift" in the "menu" region
    Then I should visibly see the link "Personalizations" in the "lift_tray" region
    When I hover over "Personalizations" in the "lift_tray" region
    And I click "Testing campaign editable" in the "lift_tray" region
    Then I should visibly see the link "What" in the "lift_tray" region
    And I should visibly see the link "Why" in the "lift_tray" region
    And I should visibly see the link "Who" in the "lift_tray" region
    And I should visibly see the link "When" in the "lift_tray" region
    And I should visibly see the link "Review" in the "lift_tray" region
    And I should visibly see the link "Reports" in the "lift_tray" region
    And menu item "Reports" should be "inactive"

    # I should see links to add, edit, and delete variations and variation set.
    When I hover over "What" in the "lift_tray" region
    And I should visibly see the link "Option A" in the "lift_tray" region
    And I should visibly see the link "Option B" in the "lift_tray" region
    And "Muppet mania" set "Control variation" variation should not have the "Edit" link
    And "Muppet mania" set "Control variation" variation should not have the "Delete" link
    And "Muppet mania" set "Option A" variation should have the "Edit" link
    And "Muppet mania" set "Option A" variation should have the "Delete" link
    And "Muppet mania" set "Option B" variation should have the "Edit" link
    And "Muppet mania" set "Option B" variation should have the "Delete" link
    And I should visibly see the link "Add variation" in the "lift_tray" region
    And I should visibly see the link "Add variation set" in the "lift_tray" region
    And I should not see the text "Personalizations that are running cannot be edited."

    # I should see links to rename, delete, and add goals.
    When I hover over "Why" in the "lift_tray" region
    Then I should see the text "Clicks Muppet mania" in the "lift_tray" region
    And I should visibly see the link "Add goal" in the "lift_tray" region
    And I should visibly see the link "Delete" in the "lift_tray" region
    And I should visibly see the link "Rename" in the "lift_tray" region
    And I should not see the text "Personalizations that are running cannot be edited."

  Scenario: Select an uneditable personalization.
    # I have a campaign with a variation set and a goal.
    # I login with the marketer role.
    # I am on an article page.
    Given "acquia_lift_target" agents:
      | machine_name                | label                       |
      | testing-campaign-uneditable | Testing campaign uneditable |
    And personalized elements:
      | label       | agent                       | selector    | type     | content                           |
      | Mahna Mahna | testing-campaign-uneditable | #page-title | editText | Do doo do do do, Mah mah mahna ma |
    And the "testing-campaign-uneditable" personalization has the "Running" status
    And I am logged in as a user with the "access administration pages,access toolbar,administer visitor actions,manage personalized content" permission
    And I am viewing an "Article" content:
      | title | Test Article Title |

    # I select the campaign.
    And I click "Acquia Lift" in the "menu" region
    Then I should visibly see the link "Personalizations" in the "lift_tray" region
    When I hover over "Personalizations" in the "lift_tray" region
    And I click "Testing campaign uneditable" in the "lift_tray" region
    Then I should visibly see the link "What" in the "lift_tray" region
    And I should visibly see the link "Why" in the "lift_tray" region
    And I should visibly see the link "Who" in the "lift_tray" region
    And I should visibly see the link "When" in the "lift_tray" region
    And I should visibly see the link "Review" in the "lift_tray" region
    And I should visibly see the link "Reports" in the "lift_tray" region
    And menu item "Reports" should be "active"

    # I should see the edit links have been disabled.
    When I hover over "What" in the "lift_tray" region
    And I should visibly see the link "Option A" in the "lift_tray" region
    And I should visibly see the link "Option B" in the "lift_tray" region
    And "Mahna Mahna" set "Control variation" variation should not have the "Edit" link
    And "Mahna Mahna" set "Control variation" variation should not have the "Delete" link
    And "Mahna Mahna" set "Option A" variation "Edit" link is disabled
    And "Mahna Mahna" set "Option A" variation "Delete" link is disabled
    And "Mahna Mahna" set "Option B" variation "Edit" link is disabled
    And "Mahna Mahna" set "Option B" variation "Delete" link is disabled
    And I should not visibly see the link "Add variation" in the "lift_tray" region
    And menu item "Add variation set" should be "inactive"
    And I should see the text "Personalizations that are running cannot be edited."

    # I should see links to rename, delete, and add goals have been disabled.
    When I hover over "Why" in the "lift_tray" region
    Then I should see the text "Clicks Mahna Mahna" in the "lift_tray" region
    And menu item "Add goal" should be "inactive"
    And menu item "Delete" should be "inactive"
    And menu item "Rename" should be "inactive"
    And I should see the text "Personalizations that are running cannot be edited."
