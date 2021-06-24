<?php

namespace Drupal\acquia_perz;

use Drupal\Core\Config\ImmutableConfig;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Entity\EntityFieldManagerInterface;
use Drupal\Core\Datetime\DateFormatterInterface;
use Drupal\Component\Uuid\UuidInterface;
use Drupal\Core\Render\RendererInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\field\Entity\FieldConfig;
use Drupal\Component\Datetime\TimeInterface;
use GuzzleHttp\Exception\TransferException;
use GuzzleHttp\ClientInterface;

/**
 * Contains helper methods for managing Content Index Service exports.
 *
 * @package Drupal\acquia_perz
 */
class ExportContent {

  const DELETED = 'deleted';

  const EXPORTED = 'exported';

  const FAILED = 'failed';

  /**
   * The http client service.
   *
   * @var \GuzzleHttp\ClientInterface
   */
  protected $httpClient;

  /**
   * The export queue service.
   *
   * @var \Drupal\acquia_perz\ExportQueue
   */
  protected $exportQueue;

  /**
   * The export tracker service.
   *
   * @var \Drupal\acquia_perz\ExportTracker
   */
  protected $exportTracker;

  /**
   * The acquia perz entity settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\ContentPublishingForm
   */
  protected $entitySettings;

  /**
   * The acquia perz cis settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\CISSettingsForm
   */
  protected $cisSettings;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * @var \Drupal\Core\Entity\EntityFieldManagerInterface
   */
  protected $entityFieldManager;

  /**
   * The renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * The uuid generator.
   *
   * @var \Drupal\Component\Uuid\UuidInterface
   */
  protected $uuidGenerator;

  /**
   * The date formatter service.
   *
   * @var \Drupal\Core\Datetime\DateFormatterInterface
   */
  protected $dateFormatter;

  /**
   * The time service.
   *
   * @var \Drupal\Component\Datetime\TimeInterface
   */
  protected $time;

  /**
   * ExportContent constructor.
   *
   * @param \GuzzleHttp\ClientInterface $http_client
   *   The http client service.
   * @param \Drupal\acquia_perz\ExportQueue $export_queue
   *   The Export Queue service.
   * @param \Drupal\acquia_perz\ExportTracker $export_tracker
   *   The Export Tracker service.
   * @param \Drupal\Core\Config\ImmutableConfig $entity_settings
   *   The acquia perz entity settings.
   * @param \Drupal\Core\Config\ImmutableConfig $cis_settings
   *   The acquia perz cis settings.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Entity\EntityFieldManagerInterface $entity_field_manager
   *   The entity type manager.
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer.
   * @param \Drupal\Component\Uuid\UuidInterface $uuid_generator
   *   The UUID generator.
   * @param \Drupal\Core\Datetime\DateFormatterInterface $date_formatter
   *   The date formatter service.
   * @param \Drupal\Component\Datetime\TimeInterface $time
   *   The time service.
   */
  public function __construct(ClientInterface $http_client, ExportQueue $export_queue, ExportTracker $export_tracker, ImmutableConfig $entity_settings, ImmutableConfig $cis_settings, EntityTypeManagerInterface $entity_type_manager, EntityFieldManagerInterface $entity_field_manager, RendererInterface $renderer, UuidInterface $uuid_generator, DateFormatterInterface $date_formatter, TimeInterface $time) {
    $this->httpClient = $http_client;
    $this->exportQueue = $export_queue;
    $this->exportTracker = $export_tracker;
    $this->entitySettings = $entity_settings;
    $this->cisSettings = $cis_settings;
    $this->entityTypeManager = $entity_type_manager;
    $this->entityFieldManager = $entity_field_manager;
    $this->renderer = $renderer;
    $this->uuidGenerator = $uuid_generator;
    $this->dateFormatter = $date_formatter;
    $this->time = $time;
  }

  /**
   * Returns the value of the entity view modes setting.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   *
   * @return array
   *   The setting value.
   */
  protected function getEntityViewModesSettingValue(EntityInterface $entity): array {
    return $this->entitySettings->get("view_modes.{$entity->getEntityTypeId()}.{$entity->bundle()}") ?? [];
  }

  /**
   * Export all entity view modes.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   */
  public function exportEntity(EntityInterface $entity) {
    $entity_type_id = $entity->getEntityTypeId();
    $entity_id = $entity->id();
    $langcode = $entity->language()->getId();
    $entity_uuid = $entity->uuid();
    $entity_payload = $this->getEntityPayload($entity_type_id, $entity_id, $langcode);
    try {
      $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.ins_upd_slow_endpoint');
      $this->sendBulk($entity_payload, $slow_mode);
      $this->exportTracker->export(
        $entity_type_id,
        $entity_id,
        $entity_uuid,
        $langcode
      );
    }
    catch (TransferException $e) {
      if ($e->getCode() === 0) {
        $this->exportTracker->exportTimeout(
          $entity_type_id,
          $entity_id,
          $entity_uuid,
          $langcode
        );
        $this->exportQueue->addBulkQueueItem(
          'insert_or_update',
          [
            [
              'entity_type_id' => $entity_type_id,
              'entity_id' => $entity_id,
              'entity_uuid' => $entity_uuid,
            ]
          ],
          $langcode
        );
        return self::FAILED;
      }
    }
  }

  /**
   * Get and export entities from the list.
   *
   * @param array $entities
   *  List of the entities that should be exported.
   * @param string $langcode
   *  Language code of the entity translation that should be exported.
   * 'all' value means that all entity translations should be exported.
   */
  public function exportEntities($entities, $langcode = 'all') {
    $entities_payload = [];
    foreach ($entities as $entity_item) {
      $entity_type_id = $entity_item['entity_type_id'];
      $entity_id = $entity_item['entity_id'];

      $entities_payload = array_merge(
        $entities_payload,
        $this->getEntityPayload($entity_type_id, $entity_id, $langcode)
      );
    }
    $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.ins_upd_slow_endpoint');
    $this->sendBulk($entities_payload, $slow_mode);
    // Track export for each entity and its languages.
    foreach ($entities as $entity_item) {
      $this->exportTracker->trackEntity(
        $entity_item['entity_type_id'],
        $entity_item['entity_id'],
        $langcode
      );
    }
    return self::EXPORTED;
  }

  /**
   * Export entity by view mode.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   * @param string $view_mode
   *   The view mode.
   * @param string $langcode
   *   The language code.
   */
  protected function getEntityVariation(EntityInterface $entity, $view_mode, $langcode) {
    $elements = $this->entityTypeManager
      ->getViewBuilder($entity->getEntityTypeId())
      ->view($entity, $view_mode, $langcode);
    $rendered_data = $this->renderer->renderPlain($elements);
    $result = [
      'content_uuid' => $entity->uuid(),
      'account_id' => $this->cisSettings->get('cis.account_id'),
      'content_type' => $entity->getEntityTypeId(),
      'view_mode' => $view_mode,
      'language' => $langcode,
      'number_view' => 0,
      'label' => $entity->label(),
      'updated' => $this->dateFormatter->format($this->time->getCurrentTime(), 'custom', 'Y-m-d\TH:i:s'),
      'rendered_data' => $rendered_data,
    ];
    $taxonomy_relations = $this->getEntityTaxonomyRelations($entity);
    if ($taxonomy_relations) {
      $result['relations'] = $taxonomy_relations;
    }
    return $result;
  }

  /**
   * Get and export entity by its entity type and id.
   *
   * @param string $entity_type_id
   *  Entity type id of the entity that should be exported.
   * @param integer $entity_id
   *  Id of the entity that should be exported.
   * @param string $entity_uuid
   *  Uuid of the entity that should be exported.
   * @param string $langcode
   *  Language code of the entity translation that should be exported.
   * 'all' value means that all entity translations should be exported.
   */
  public function exportEntityById($entity_type_id, $entity_id, $langcode = 'all') {
    $entity_payload = $this->getEntityPayload($entity_type_id, $entity_id, $langcode);
    $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.ins_upd_slow_endpoint');
    $this->sendBulk($entity_payload, $slow_mode);
    // @TODO tracking.
    return self::EXPORTED;
  }

  /**
   * @param $entity_type_id
   * @param $entity_id
   * @param $langcode
   *
   * @return array|void
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function getEntityPayload($entity_type_id, $entity_id, $langcode = 'all') {
    $payload = [];
    $entity = $this
      ->entityTypeManager
      ->getStorage($entity_type_id)
      ->load($entity_id);
    if (!$entity instanceof ContentEntityInterface) {
      return [];
    }
    if (!$view_modes = $this->getEntityViewModesSettingValue($entity)) {
      return [];
    }
    foreach (array_keys($view_modes) as $view_mode) {
      // The preview image field setting is saved along side the view modes.
      // Don't process it as one.
      if ($view_mode == 'acquia_perz_preview_image') {
        continue;
      }
      if ($langcode === 'all') {
        foreach ($entity->getTranslationLanguages() as $language) {
          $language_id = $language->getId();
          $translation = $entity->getTranslation($language_id);
          $payload[] = $this->getEntityVariation($translation, $view_mode, $language_id);
        }
      }
      else {
        $translation = $entity->getTranslation($langcode);
        $payload[] = $this->getEntityVariation($translation, $view_mode, $langcode);
      }
    }
    return $payload;
  }

  /**
   * Delete entity.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   */
  public function deleteEntity(EntityInterface $entity) {
    $entity_type_id = $entity->getEntityTypeId();
    $entity_id = $entity->id();
    $langcode = $entity->language()->getId();
    $entity_payload = $this->getEntityPayload($entity_type_id, $entity_id, $langcode);
    try {
      $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.delete_entity_slow_endpoint');
      $this->send('DELETE', $entity_payload, $slow_mode);
      //$this->exportTracker->delete($entity_type_id, $entity_id, $entity_uuid);
    }
    catch (TransferException $e) {
      if ($e->getCode() === 0) {
        /*$this->exportTracker->deleteTimeout(
          $entity_type,
          $entity_id,
          $entity_uuid
        );*/
        $this->exportQueue->addBulkQueueItem(
          'delete_entity',
          [
            [
              'entity_type_id' => $entity_type_id,
              'entity_id' => $entity_id,
              'entity_uuid' => $entity->uuid(),
            ]
          ]
        );
        return self::FAILED;
      }
    }
  }

  /**
   * Delete entity by its entity type and id.
   *
   * @param string $entity_type_id
   *  Entity type id of the entity that should be deleted.
   * @param integer $entity_id
   *  Id of the entity that should be deleted.
   * @param string $entity_uuid
   *  Uuid of the entity that should be deleted.
   */
  public function deleteEntityById($entity_type_id, $entity_id, $entity_uuid) {
    $entity_payload = $this->getEntityPayload($entity_type_id, $entity_id);
    $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.delete_entity_slow_endpoint');
    $this->send('DELETE', $entity_payload, $slow_mode);
    /*$this->exportTracker->delete(
      $entity_type,
      $entity_id,
      $entity_uuid
    );*/
  }

  /**
   * Delete translation.
   *
   * @param \Drupal\Core\Entity\EntityInterface $translation
   *   The current entity translation.
   * @param string $langcode
   *  Language code of the entity translation that should be deleted.
   */
  public function deleteTranslation(EntityInterface $translation, $langcode = '') {
    $entity_type_id = $translation->getEntityTypeId();
    $entity_id = $translation->id();
    $entity_payload = $this->getEntityPayload($entity_type_id, $entity_id, $langcode);
    try {
      $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.delete_translation_slow_endpoint');
      $this->send('DELETE', $entity_payload, $slow_mode);
      //$this->exportTracker->delete($entity_type, $entity_id, $entity_uuid, $langcode);
    }
    catch (TransferException $e) {
      if ($e->getCode() === 0) {
        /*$this->exportTracker->deleteTimeout(
          $entity_type,
          $entity_id,
          $entity_uuid,
          $langcode
        );*/
        $this->exportQueue->addBulkQueueItem(
          'delete_translation',
          [
            [
              'entity_type_id' => $entity_type_id,
              'entity_id' => $entity_id,
              'entity_uuid' => $translation->uuid(),
            ]
          ]
        );
        return self::FAILED;
      }
    }
  }

  /**
   * Delete translation by its entity type, entity id and langcode.
   *
   * @param string $entity_type_id
   *  Entity type id of the entity that should be deleted.
   * @param integer $entity_id
   *  Id of the entity that should be deleted.
   * @param string $entity_uuid
   *   The entity uuid of the entity that should be deleted.
   * @param string $langcode
   *  Language code of the entity translation that should be deleted.
   */
  public function deleteTranslationById($entity_type_id, $entity_id, $entity_uuid, $langcode) {
    $entity_payload = $this->getEntityPayload($entity_type_id, $entity_id, $langcode);
    $slow_mode = \Drupal::config('acquia_perz.settings')->get('cis.delete_translation_slow_endpoint');
    $this->send('DELETE', $entity_payload, $slow_mode);
    //$this->exportTracker->delete($entity_type, $entity_id, $entity_uuid);
  }

  /**
   * Send bulk request to CIS.
   * @param string $method
   *  The sending method.
   * @param array $data
   *  The data that should be sent to CIS.
   * @param false $slow_request_test
   */
  protected function sendBulk($json, $slow_request_test = FALSE) {
    if ($slow_request_test) {
      $username = 'admin';
      $password = 'admin';
      $client_headers = [
        'Accept' => 'application/haljson',
        'Content-Type' => 'application/haljson',
        'Authorization' => 'Basic ' . base64_encode("$username:$password"),
      ];
      $host = \Drupal::request()->getSchemeAndHttpHost();
      $url = $host . '/api/slow_endpoint';
      $response = \Drupal::service('http_client')->request('GET',
        $url, [
          'headers' => $client_headers,
          'timeout' => 2,
        ]
      );
      return self::FAILED;
    }
    else {
      $client_headers = [
        'Content-Type' => 'application/json',
        'Accept' => 'application/json',
      ];
      $url = $this->cisSettings->get('cis.endpoint');
      $query_string = http_build_query([
        'environment' => $this->cisSettings->get('cis.environment'),
        'origin' => $this->cisSettings->get('cis.origin'),
      ]);
      $url .= "?{$query_string}";
      $response = $this->httpClient->request('PUT',
        $url, [
          'headers' => $client_headers,
          'timeout' => $this->cisSettings->get('cis.endpoint_timeout', 2),
          'body' => json_encode($json),
        ]
      );
      return self::EXPORTED;
    }
  }

  /**
   * Send request to CIS.
   * @param string $method
   *  The sending method.
   * @param array $data
   *  The data that should be sent to CIS.
   * @param false $slow_request_test
   */
  protected function send($method, $entity_uuid, $json, $slow_request_test = FALSE) {
    if ($slow_request_test) {
      $username = 'admin';
      $password = 'admin';
      $client_headers = [
        'Accept' => 'application/haljson',
        'Content-Type' => 'application/haljson',
        'Authorization' => 'Basic ' . base64_encode("$username:$password"),
      ];
      $host = \Drupal::request()->getSchemeAndHttpHost();
      $url = $host . '/api/slow_endpoint';
      $response = \Drupal::service('http_client')->request('GET',
        $url, [
          'headers' => $client_headers,
          'timeout' => 2,
        ]
      );
      return self::FAILED;
    }
    elseif ($method !== 'DELETE') {

      $client_headers = [
        'Content-Type' => 'application/json',
        'Accept' => 'application/json',
      ];
      $url = $this->cisSettings->get('cis.endpoint');
      $query_string = http_build_query([
        'environment' => $this->cisSettings->get('cis.environment'),
        'origin' => 'abcd',
      ]);
      $url .= "/{$entity_uuid}?{$query_string}";
      $response = $this->httpClient->request($method,
        $url, [
          'headers' => $client_headers,
          'timeout' => 2,
          'body' => json_encode($json),
        ]
      );
      return self::EXPORTED;
    }
  }

  /**
   * Returns array of related taxonomy term fields and their corresponding term
   * uuids; only taxonomies that are checked on Entity settings form.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *
   * @return array
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  protected function getEntityTaxonomyRelations(EntityInterface $entity) {
    $relations = [];
    $entity_type_id = $entity->getEntityTypeId();
    $bundle = $entity->bundle();
    $view_modes = $this->entitySettings->get('view_modes');
    $available_taxonomies = [];
    if (isset($view_modes['taxonomy_term'])) {
      $available_taxonomies = array_keys($view_modes['taxonomy_term']);
    }
    $fields = $this->entityFieldManager
      ->getFieldDefinitions($entity_type_id, $bundle);
    foreach ($fields as $field) {
      if ($field instanceof FieldConfig
        && $field->getType() === 'entity_reference'
        && $field->getSetting('handler') === 'default:taxonomy_term'
      ) {
        $field_name = $field->getName();
        $settings = $field->getSetting('handler_settings');
        $field_taxonomies = $settings['target_bundles'];
        // Check if field contains at least one available taxonomy.
        if (count(array_intersect($available_taxonomies, $field_taxonomies)) == 0) {
          continue;
        }
        $terms = $entity->get($field_name)->getValue();
        foreach ($terms as $term) {
          $term_entity = $this->entityTypeManager
            ->getStorage('taxonomy_term')
            ->load($term['target_id']);
          $term_uuid = $term_entity->uuid();
          if (in_array($term_entity->bundle(), $available_taxonomies)) {
            if (isset($relations[$field_name])) {
              $relations[$field_name][] = $term_uuid;
            }
            else {
              $relations[$field_name] = [$term_uuid];
            }
          }
        }
      }
    }
    return $relations;
  }

}
