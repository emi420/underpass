CREATE INDEX nodes_version_idx ON public.nodes (version);
CREATE INDEX ways_poly_version_idx ON public.ways_poly (version);
CREATE INDEX ways_line_version_idx ON public.ways_line (version);
CREATE INDEX idx_changesets_hashtags ON public.changesets USING gin(hashtags);

ALTER TABLE ONLY public.ways_poly
    ADD CONSTRAINT ways_poly_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.ways_line
    ADD CONSTRAINT ways_line_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.nodes
    ADD CONSTRAINT nodes_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.relations
    ADD CONSTRAINT relations_pkey PRIMARY KEY (osm_id);

