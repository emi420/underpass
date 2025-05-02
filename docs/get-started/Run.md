## Keep the database up-to-date minutely

1. Download OSM PBF and GeoJSON priority boundary files
2. Run `underpass -s <DB> -i <PBF file path> -b <GeoJSON priority boundary>`

Example:

```bash
wget https://download.geofabrik.de/europe/andorra-latest.osm.pbf
wget https://download.geofabrik.de/europe/andorra.poly
python utils/poly2geojson.py andorra.poly
underpass -i andorra-latest.osm.pbf -s localhost/underpass -b andorra.geojson
```

If the process has stopped, you can continue from latest processed timestamp:

```bash
underpass --latest -s localhost/underpass -b andorra.geojson
```

