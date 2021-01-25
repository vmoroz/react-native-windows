---
id: {{docId}}
title: {{typeName}}
---

{{&summary}}

## Definition

```cpp
{{&prototype}}
```

Defined in `{{fileName}}`  
Namespace: **`{{namespace}}`**  
{{#namespaceAlias}}
Namespace alias: **`{{.}}`**
{{/namespaceAlias}}

{{#sections}}

### {{title}}

Name | Description
-----|------------
{{#memberOverloads}}[**`{{&name}}`**]({{anchor}}) | {{&summary}}
{{/memberOverloads}}
{{/sections}}

{{#details}}

### Notes

{{&details}}
{{/details}}

{{#sections}}
{{#memberOverloads}}

---

## `{{&name}}`

{{#members}}

```cpp
{{&prototype}}
```

{{&brief}}

{{&details}}
{{/members}}

{{/memberOverloads}}
{{/sections}}
