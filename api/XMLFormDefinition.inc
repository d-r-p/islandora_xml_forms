<?php

// $Id$

/**
 * @file
 * @author Nigel Banks
 *
 * Defines a FormDefinition class that allows XML form defintions to be processed by the FormGenerator, and other
 * classes.
 */
module_load_include('inc', 'php_lib', 'String');
module_load_include('inc', 'php_lib', 'Array');

/**
 * Describes a form
 */
class XMLFormDefinition {

  /**
   * Checks to see if a xml form definition is valid.
   * 
   * @param mixed $definition
   *   The xml form definition to validated. Can be a string a DOMDocument or a SimpleXMLElement
   * @return boolean
   *   TRUE if $definition is a valid form definition, FALSE otherwise.
   */
  public static function isValid(&$definition) {
    $path = drupal_get_path('module', 'xml_form_api') . '/XMLFormDefinition.xsd';
    if (is_string($definition)) {
      $document = new DOMDocument();
      $document->loadXML($definition);
      return $document->schemaValidate($path);
    }
    else if (get_class($definition) == 'DOMDocument') {
      return $definition->schemaValidate($path);
    }
    else if (get_class($definition) == 'SimpleXMLElement') {
      $form = dom_import_simplexml($definition);
      $document = new DOMDocument('1.0');
      $form = $document->importNode($form, true);
      $form = $document->appendChild($form);
      return $document->schemaValidate($path);
    }
    return FALSE;
  }

  /**
   * Create
   */
  public static function Create(array $properties, array $form) {
    $definition = new SimpleXMLElement('<definition xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"></definition>');
    self::AddPropertiesToDefinition($definition, $properties);
    echo $definition->asXML();
    exit();
    self::AddFormToDefinition($definition, new FormElement($form));
    return dom_import_simplexml($definition);
  }

  /**
   * 
   * @param SimpleXMLElement $definition 
   * @param array $properties
   */
  private static function AddPropertiesToDefinition(SimpleXMLElement $definition, array &$properties) {
    $form_properties = $definition->addChild('properties');
    if (isset($properties['document'])) {
      $document_properties = $form_properties->addChild('document');
      if (isset($properties['document']['root'])) {
        $root = $document_properties->addChild('root');
        if (isset($properties['document']['root']['prefix'])) {
          $root->addChild('prefix', $properties['document']['root']['prefix']);
        }
        if (isset($properties['document']['root']['local_name'])) {
          $root->addChild('local_name', $properties['document']['root']['local_name']);
        }
        if (isset($properties['document']['root']['uri'])) {
          $root->addChild('uri', $properties['document']['root']['uri']);
        }
      }
      if (isset($properties['document']['namespaces'])) {
        $namespaces = $document_properties->addChild('namespaces');
        foreach ($properties['document']['namespaces'] as $item) {
          $namespace = $namespaces->addChild('namespace', $item['uri']);
          $namespace->addAttribute('prefix', $item['prefix']);
        }
      }
      if (isset($properties['document']['schema'])) {
        $document_properties->addChild('schema', $properties['document']['schema']);
      }
    }
  }

  /**
   *
   * @param SimpleXMLElement $definition
   * @param FormElement $form
   */
  private static function AddFormToDefinition(SimpleXMLElement $definition, FormElement $form) {
    
  }

  /**
   * Get form properties.
   * 
   * @param sting $xml_definition
   * 
   * @return string 
   */
  public static function GetFormProperties(&$xml_definition) {
    if (!self::isValid($xml_definition)) {
      throw new Exception('The given XMLFormDefinition is invalid.');
    }
    $definition = simplexml_load_string($xml_definition);
    $document = $definition->properties->document;
    $root = array(
      'prefix' => (string) $document->root->prefix,
      'local_name' => (string) $document->root->local_name,
      'uri' => (string) $document->root->uri,
    );
    $namespaces = array();
    if (isset($document->namespaces)) {
      $children = $document->namespaces->children();
      foreach ($children as $namespace) {
        $prefix = (string) $namespace['prefix'];
        $namespaces[$prefix] = (string) $namespace;
      }
    }
    $schema = (string) $definition->properties->document->schema;
    return array(
      'document' => array(
        'root' => $root,
        'namespaces' => $namespaces,
        'schema' => $schema
    ));
  }

  /**
   * Get a drupal form definition from the given XML based one.
   * 
   * @param string $xml_definition 
   * 
   * @return array
   */
  public static function GetDrupalForm(&$xml_definition) {
    if (!self::isValid($xml_definition)) {
      throw new Exception('The given XMLFormDefinition is invalid.');
    }
    $definition = simplexml_load_string($xml_definition);
    return self::GetElement($definition->form);
  }

  /**
   *
   * @param SimpleXMLElement $element
   * @return type 
   */
  private static function GetElementProperties(SimpleXMLElement $element) {
    $output = array();
    if (isset($element->properties)) {
      $properties = $element->properties->children();
      foreach ($properties as $property) {
        $name = $property->getName();
        $output["#$name"] = self::GetElementProperty($property);
      }
    }
    return $output;
  }

  /**
   *
   * @param SimpleXMLElement $property
   * @return type 
   */
  private static function GetElementProperty(SimpleXMLElement $property) {
    $children = $property->children();
    if (count($children) == 0) {
      return cast_string_to_boolean((string) $property); // If cast fails the string is returned.
    }
    $output = array();
    foreach ($children as $child) {
      $name = self::GetElementPropertyName($child);
      $output[$name] = self::GetElementProperty($child);
    }
    return $output;
  }

  private static function GetElementPropertyName(SimpleXMLElement $property) {
    if (isset($property['key'])) {
      $key = (string) $property['key'];
      return strcasecmp($key, 'NULL') == 0 ? NULL : $key;
    }
    return $property->getName();
  }

  private static function GetElementChildren(SimpleXMLElement $element) {
    $output = array();
    if (isset($element->children)) {
      $elements = $element->children->children();
      foreach ($elements as $element) {
        $key = isset($element['name']) ? (string) $element['name'] : NULL;
        array_add($output, $key, self::GetElement($element));
      }
    }
    return $output;
  }

  private static function GetElement(SimpleXMLElement $element) {
    $output = self::GetElementProperties($element);
    return array_merge($output, self::GetElementChildren($element));
  }

}
