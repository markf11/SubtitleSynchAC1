# Garnier vallomásának célzott vizsgálata – v5.16

A diagnosztika alapból kikapcsolva marad. A feliratlejátszás, a pause-kezelés,
a méret és az időzítések viselkedése nem változik.

## Rövid játékbeli felvétel

1. A friss ASI telepítése után indítsd újra a játékot.
2. Garnier megölése **előtt** nyomj F1-et és kapcsold be a
   **Record Garnier confession** jelölőnégyzetet. Zárd be az F1-ablakot.
3. Játszd végig a teljes vallomást. Egy hibás vagy kimaradó mondat **kezdetén**
   nyomj F2-t. A képernyőn látható sorszám segít a videó és a napló párosításában.
   Többszöri F2 nem változtatja meg a feliratot; az elemző csoportosítja az
   ismételt jelöléseket, de nem dönti el helyetted, melyik volt téves nyomás.
4. A vallomás végén F1-ben kapcsold ki a **Record Garnier confession** jelölést.
   Ekkor mindkét fájl lezárul és kiíródik a lemezre. Nem kell kilépni a játékból.
5. Küldd el a játék `scripts` mappájából ezeket:
   - `SubtitleSynchAC1.garnier.log`
   - `SubtitleSynchAC1.garnier.capture.tsv`
   - az ezen a gépen használt `subtitles.json`

Ha van videó, az F2-sorszámokkal megjelölt rész különösen hasznos. A napló
hangazonosítókat tartalmaz, nem hangfelvételt és nem automatikus beszédfelismerést.

A Garnier-fájlok elkülönülnek a korábbi általános logoktól. Új felvétel mindig
hozzáfűződik, külön sessionnel; az előző felvétel megmarad. Minden közeli
hangeseményt rögzítünk, mert a még ismeretlen beszéd-ID-ket nem lehet előre
biztonságosan kiszűrni. Az általános és a célzott felvétel között váltás lezárja
az előző sessiont. Sikertelen váltáskor a régi rögzítés tovább működik.

## Mit mutat a diagnosztika?

- Nyers és feloldott hangazonosító, hook-értesítés ideje, adatbázis-találat.
- Feliratindítás, az összes megfigyelt szövegrész váltása és feliratvég.
- Eltűnés oka: időtartam lejárt, másik felirat felülírta, menü eltakarta.
- Hiányzó/üres adatbázisbejegyzés, feloldatlan hangesemény, pause miatti
  pufferelés vagy eldobás, megtelt hangeseménysor.
- Egy képkockán belül átugrott szövegrészek külön jelzése.
- A játékba visszatérés és a tényleges láthatóság változásai.

Az értesítés ideje a megfigyelt hook időpontja; ez nem igazolja a hang tényleges
kezdő- és végpontját. A felirat szövegének helyességét hang/videó alapján kell
ellenőrizni. A diagnosztika nem találgat ID-eltolást és nem ír át feliratokat.

## Elemzés

```powershell
python tools/review_garnier_capture.py "SubtitleSynchAC1.garnier.capture.tsv" --database "subtitles.json" --output "garnier-review"
```

Az eszköz `garnier-review.md` és `garnier-review.json` fájlt készít.
Alapból az utolsó Garnier-sessiont vizsgálja; `--session 2` például a másodikat.
Az egyes sessionöket és ugyanazon ID új lejátszásait külön kezeli.
A jelentés jelzi a még nem párosított mondatokat, a túl korai feliratvéget,
a meg nem jelent szövegrészeket, valamint az F2 körüli hangjelölteket.
A korai végpontot az adatbázis időzítéséhez hasonlítja, nem a meghallgatott hanghoz.

A szövegkönyv 41–42. oldalából készült ellenőrzőlista a
`localization/garnier-confession-reference.json` fájlban található. Az ismert
három Garnier-bejegyzés ID-ja szerepel benne; a többi mondat ID-ja egyelőre
ismeretlen. Számozási hézagokból nem hozunk létre új párosítást.

## Install and restore

This public source/CI publication builds an ASI-only runtime update. With the game
closed, back up and replace scripts/SubtitleSynchAC1.asi, retaining your installed
subtitle JSON, font, loader and INI. The separately distributed Hungarian
translation package is not required for the recording profile. Restore the old
ASI with the game closed to roll back. No game archives are included here.
