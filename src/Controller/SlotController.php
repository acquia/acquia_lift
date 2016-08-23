<?php

namespace Drupal\acquia_lift\Controller;

use Drupal\Component\Render\FormattableMarkup;
use Drupal\Core\Controller\ControllerBase;
use Drupal\acquia_lift\SlotInterface;

/**
 * Provides route responses for facets.
 */
class SlotController extends ControllerBase {

    /**
     * Displays information about an Acquia Lift Slot.
     *
     * @param \Drupal\acquia_lift\SlotInterface $slot
     *   The slot to display.
     *
     * @return array
     *   An array suitable for drupal_render().
     */
    public function page(SlotInterface $slot) {
        // Build the search index information.
        $render = array(
          'view' => array(
            '#theme' => 'acquia_lift_slot',
            '#slot' => $slot,
          ),
        );
        return $render;
    }

    /**
     * Returns the page title for an slot's "View" tab.
     *
     * @param \Drupal\acquia_lift\SlotInterface $slot
     *   The facet that is displayed.
     *
     * @return string
     *   The page title.
     */
    public function pageTitle(SlotInterface $slot) {
        return new FormattableMarkup('@title', array('@title' => $slot->label()));
    }
}
