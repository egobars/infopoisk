# Обратный индекс

Обратный индекс на основе ЛСМ-дерева для поиска документов по словам на английском языке.

# Использование

Объект индекса создаётся также, как объект LSM-дерева:

```
Index(min_degree, max_components, component_size_multiplier)
```

Документ в индекс добавляется при помощи функции ```AddDocument```.

Объект поиска создаётся из индекса и слова, по которому надо найти документы:

```
Finder(Index& index, std::string word)
```

Можно добавлять условия к объекту поиска при помощи функций ```Add```, ```Or``` и ```Without```.
Результат получается функцией ```GetDocuments```.

# Использованные библиотеки

Для roaring-bitmaps: https://github.com/RoaringBitmap/CRoaring

Для стемматизации: https://github.com/Blake-Madden/OleanderStemmingLibrary
