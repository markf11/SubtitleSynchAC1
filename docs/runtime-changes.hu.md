# A mod funkciói és javításai – v5.16

Ez a lista a magyar szövegek és UI-fájlok fordításától független változásokat
foglalja össze az eredeti bloxtbc/SubtitleSynchAC1 kódhoz képest.

## Hangazonosítás és stabilitás

- Ellenőrzött hangesemény–hangforrás párosítás javítja a hibás Animus-feliratokat.
  A mod az eredeti hangazonosítót részesíti előnyben, és nem találgat ID-eltolást.
- A hookmintáknak egyetlen találatot kell adniuk a végrehajtható kódszakaszban.
  Hiányzó vagy többértelmű találatnál az érintett hook nem települ.
- Megmaradnak az eredeti függvényhívások mellékhatásai, visszatérési értékei,
  regiszterei, flagjei és lebegőpontos/SIMD állapota.
- Az időkorlát nélküli utolsó-ID-szűrés megszűnt: ugyanaz a hang új lejátszáskor
  ismét elindíthatja a feliratot.
- A hangesemények több szálról is érkezhetnek; a korlátos sor eldobásai naplózhatók.
- Vilmos memory corridor jelenetének igazolt hangazonosítóihoz feliratbejegyzések
  készültek. Ez külön ellenőrzött adatpótlás, nem általános ID-eltolási szabály.

## Szüneteltetés

- A tényleges pause menü állapota állítja meg a felirat óráját; az Escape,
  egérrel kiválasztott Resume és a kontrolleres visszatérés is kezelhető.
- A menüben a felirat elrejtőzik, de megőrzi az aktuális szövegrészt és hátralévő
  idejét. A pause határán érkező párbeszédeseményeket a mod puffereli.
- Minden visszatérés azonos, INI-ből állítható késleltetést használ; az alap 500 ms.
- A tutorial szkriptelt játékidő-megállítása elkülönül a pause menütől, ezért
  az Animus magyarázatának feliratai ezek alatt továbbhaladnak.

## Megjelenítés és beállítások

- Automatikus betűméretezés a játékablak magassága alapján, állítható alapmérettel
  és F1-es méretszorzóval. Felbontásváltáskor a méret és a tördelés frissül.
- A betűkészlet tényleges méretei alapján automatikus sortörés készül.
  Minden sor külön középre igazított, és saját szélességű háttérdobozt kap.
- A maximális szélesség alapból a játékablak 65%-a; a blokk alulról felfelé nő.
- A mentési aktivitás ideiglenesen feljebb emeli a feliratot; az alap tartás 3300 ms.
- Desmond interakciós felszólítását a tényleges kirajzolt szöveg alapján érzékeli.
  A korábbi pirosképpont-heurisztika és GPU-képolvasás megszűnt.
- Minden felirat alapból 1500 ms végső hosszabbítást kap; új felirat felülírhatja.
  A runtime nem írja át a JSON eredeti wait/duration időzítéseit.
- Az F1-ben egy méretezési csúszka maradt, a haladó pozíció- és méretbeállítások
  az INI-ben érhetők el. Az F1 megnyitása megőrzi az aktív párbeszédet.
- Az INI mentése megtartja a más szekciókban lévő beállításokat is.
- Az opcionális másodlagos/környezeti felirat nem írhat felül aktív küldetésfeliratot.

## Diagnosztika

- Alapból kikapcsolt, F1-ből élőben indítható és leállítható naplózás.
- Az F2 számozott, látható jelölést ad a videó és a napló összepárosításához.
- Rögzíthető a nyers/feloldott hang-ID, adatbázis-találat, felirat- és szegmensváltás,
  megjelenés/eltűnés, felülírási ok, pause-pufferelés és az eseménysor vesztesége.
- A Garnier-vallomás külön felvételi módot és fájlokat kapott. Az elemző
  külön kezeli a sessionöket és ismételt lejátszásokat, csoportosítja az F2-ket,
  és jelzi a gyanúsan korai feliratvéget vagy meg nem figyelt szövegrészeket.
- A hook ideje nem azonos bizonyítottan a hallható hang kezdetével/végével.
  Ismeretlen mondatot csak hanggal/videóval ellenőrzött párosítás alapján javítunk.

## Letöltés és ellenőrzés

- [Telepíthető ASI-frissítés – v5.16](https://github.com/markf11/SubtitleSynchAC1/releases/tag/v5.16)
- [Sikeres GitHub-build és tesztek](https://github.com/markf11/SubtitleSynchAC1/actions/runs/38295544192):
  8 natív x86 és 7 Python-teszt.
- [Angol változáslista](runtime-changes.md), [forrásellenőrzési útmutató](source-review.md)
  és [fordítási leírás](build-and-verify.md).

A kiadás a meglévő mod ASI-frissítése, nem teljes magyarítás. A kiadás címkéje
az ellenőrzött CI-forrásra mutat; a csomag forrásazonosítót, buildadatokat és
SHA-256 összegeket tartalmaz. A teljes forrásból saját build is készíthető.
A DX9-et vizsgáltuk részletesen; a DX10 játékbeli ellenőrzése nem azonos szintű.
Garnier hiányzó sorainak javítása még vizsgálat alatt áll. Az ellenőrzőösszeg
nem malware-tanúsítvány. Az eredeti bloxtbc/Blox MIT licenc és szerzői jelölés megmaradt.
