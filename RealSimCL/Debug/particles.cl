typedef float4 point;
typedef float4 vector;
typedef float4 color;
typedef float Density;
typedef float4 sphere;		// x, y, z, radius

typedef struct table{
	int idx;
	int neighbours[512 * 8];
}table;


float
vec4_x_vec4(vector a, vector b){
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

vector
Bounce( vector in, vector n )
{
	vector out = (in - n*(vector)( 2.0 * dot(in.xyz, n.xyz) ));
	out.w = 0.;
	return out;
}

vector
BounceSphere( point p, vector v, sphere s )
{
	vector n;
	n.xyz = fast_normalize( p.xyz - s.xyz );
	n.w = 0.;
	return Bounce( v, n );
}

bool
IsInsideSphere( point p, sphere s )
{
	float r = fast_length( p.xyz - s.xyz );
	return  ( r < s.w );
}

//
// get current smoothing radius status
//
void smoothing_table(int NUM_PARTICLES, struct table *tab, point p, global point *dPobj, float box_size){
	// get x y z range
	float x_range = p.x + box_size / 2.f;
	float y_range = p.y + box_size / 2.f;
	float z_range = p.z + box_size / 2.f;

	int count = 0;

	// find particles in close range
	for (int j = 0; j < NUM_PARTICLES; j++) {

		float posx = dPobj[j].x;
		float posy = dPobj[j].y;
		float posz = dPobj[j].z;

		// if in the range
		if ((x_range - box_size < posx && posx < x_range) && (y_range - box_size < posy && posy < y_range) && (z_range - box_size < posz && posz < z_range)) {

			// get neighbour id
			tab[0].neighbours[count] = j;
			count += 1;
		}
		else continue;

	}

	// set Table value
	tab[0].idx = count;
}

//
// get solid velocity
//
vector solid_particle_v(int closest_id, global point *dPobj, global vector *dVel, int gid){
	vector normal;

	// get fluid tan velocity
	normal.xyz = fast_normalize( dPobj[closest_id].xyz );
	normal.w = 0.f;

	vector v_fluid = dVel[closest_id];
	vector v_fluid_tan;
	v_fluid_tan = v_fluid - ( v_fluid.x * normal.x + v_fluid.y * normal.y + v_fluid.z * normal.z ) * normal;
	v_fluid_tan.w = 0.f;

	// get solid normal velocity
	normal.xyz = fast_normalize( dPobj[gid].xyz );
	normal.w = 0.f;

	vector v_solid = dVel[gid];
	vector v_solid_n;
	v_solid_n = ( v_solid.x * normal.x + v_solid.y * normal.y + v_solid.z * normal.z ) * normal;
	v_solid_n.w = 0.f;

	return - v_solid_n + v_fluid_tan;		// reverse normal
}


kernel
void
Particle( global point *dPobj, global vector *dVel, global color *dCobj, global Density *dDensity )		// position, velocity, color
{
	float4 g;
	const float		dTime = 0.005;		// 0.005 - 0.01
	const sphere	Sphere1 = (sphere)( 0., 0., 0.,  150. );

	int		gid = get_global_id( 0 );
	float	pi = 3.1415926;
	int		NUM_PARTICLES = 512*16;
	float	box_size = 20.0;			// 15 - 20
	struct	table tab[1];
	float	mass = 0.2;					// particle mass
	float	k_constant = 0.0000001;		// 100 - 1000 as book said
	float	rest_density_for_liquid = 20.;
	float	viscosity_constant = 0.0000000002f;

	// get position and velocity and density
	point	p = dPobj[gid];
	vector	v = dVel[gid];
	Density	ds = dDensity[gid];

	////////////////////////////////////////////////////// for debug, one region should got color changed
	////////////////////////////////////////////////////if (gid == 0){
	////////////////////////////////////////////////////	// get table
	////////////////////////////////////////////////////	smoothing_table(NUM_PARTICLES, tab, p, dPobj, box_size);
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////	for (int i = 0; i < tab[0].idx; i++){
	////////////////////////////////////////////////////		dCobj[tab[0].neighbours[i]] = (vector)(.9, .6, .3, 1.);
	////////////////////////////////////////////////////	}
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////}


	/////// get neighbours table
	smoothing_table(NUM_PARTICLES, tab, p, dPobj, box_size);
	

	// if is fluid
	if (gid < 4096 && gid > 0) {
    dCobj[gid].z -= 0.001;
		/////// compute fluid density
		float sum_density = 0.f;

		// sum up density kernel
		for (int n = 0; n < tab[0].idx; n++){
			
			// ri - rj, location difference
			point	ri = p;
			point	rj = dPobj[ tab[0].neighbours[n] ];
			vector	r = ri - rj;
		
			// --------density kernel, h^2 - r^2
			sum_density = sum_density + (box_size * box_size - (r.x * r.x + r.y * r.y + r.z * r.z));

		}

		float Wpoly6 = 315.0f / (64.0f * pi * pow(box_size, 9));
		
		ds = Wpoly6 * mass * sum_density;


		/////// compute fluid pressure and viscosity
		vector	sum_pressure_kernel = (vector)(0.f, 0.f, 0.f, 0.f);
		float	Wspiky = 15.0f / (pi * pow(box_size, 6));
		vector	sum_viscosity_kernel = (vector)(0.f, 0.f, 0.f, 0.f);
		float	Wviscosity = 45.0f / (pi * pow(box_size, 6));

		for (int n = 0; n < tab[0].idx; n++){
			
			// ignore 0 density
			if (dDensity[ tab[0].neighbours[n] ] == 0) continue;

			// ri - rj, location difference
			point	ri = p;
			point	rj = dPobj[ tab[0].neighbours[n] ];
			vector	r = ri - rj;

			// --------pressure kernel
			float pressure_i = k_constant * (ds - rest_density_for_liquid);
			float pressure_j = k_constant * (dDensity[ tab[0].neighbours[n] ] - rest_density_for_liquid);

			vector h_minus_r = (vector)(box_size, box_size, box_size, 0.) - r;
			h_minus_r.w = 0.;
			sum_pressure_kernel = sum_pressure_kernel + (h_minus_r * h_minus_r * h_minus_r) * (float)((pressure_i + pressure_j) / 2. / dDensity[ tab[0].neighbours[n] ]);


			// --------viscosity kernel
			float distance = sqrt(vec4_x_vec4(r, r));
			sum_viscosity_kernel = sum_viscosity_kernel + (box_size - distance) * (dVel[ tab[0].neighbours[n] ] - v) / dDensity[ tab[0].neighbours[n] ] * 0.5f;

		}

		vector f_pressure = -Wspiky * mass * sum_pressure_kernel;
		f_pressure.w = 0.f;

		vector viscosity = viscosity_constant * mass * sum_viscosity_kernel;
		viscosity.w = 0.f;

		
		/////// update 1 dTime

		vector G = (ds + 0.00000001f) * 1000000000.f * (vector)(0., -9.8, 0., 0.);

		vector f = G + f_pressure * 0.5f;
		//vector v_end = v + f/mass*dTime;
		point p_end = p + v*dTime + f/mass*(point)(0.5*dTime*dTime);
		vector v_end = v + f/mass*dTime + viscosity * 1.25f;

		p_end.w = 1.f;
		v_end.w = 0.f;

		// if collision
    float minD = 10000.f;
    int minId = -1;
    for (int i = NUM_PARTICLES / 2; i < NUM_PARTICLES; i++) {
      vector r = p_end - dPobj[i];
      float d = sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
      if (d < minD) {
        minD = d;
        minId = i;
      }
    }
    if (minD < 20) {
      vector r = p_end - dPobj[minId];
      vector n = dCobj[minId];
      if (dot(r.xyz, n.xyz) < 0) {
        // v_end = BounceSphere(p, v, Sphere1);
        v_end = Bounce(v, n);
        p_end = p + v_end*dTime + f/mass*(point)(0.5*dTime*dTime);
      }
    }
		// if (IsInsideSphere(p_end, Sphere1)){
		// 	v_end = BounceSphere(p, v, Sphere1);
		// 	p_end = p + v_end*dTime + f/mass*(point)(0.5*dTime*dTime);
		// }

		// ignore if below -500
		if (p_end.y < -500){
			v_end = (vector)(0., 0., 0., 0.);
			dCobj[gid] = (color)(0., 0., 0., 1.);
		}

		dPobj[gid] = p_end;
		dVel[gid] = v_end;
		dDensity[gid] = ds;

	}
	// if is solid
	else{
		
		// find closest liquid particle
		int closest_id = -1;
		float dist = 10000.f;			// box size is < 100
		
		for (int n = 0; n < tab[0].idx; n++){
			
			if (tab[0].neighbours[n] < 512*8){
				
				// get length, ri - rj, location difference
				point	ri = p;
				point	rj = dPobj[ tab[0].neighbours[n] ];
				vector	r = ri - rj;
				float length = sqrt( r.x * r.x + r.y * r.y + r.z * r.z );
		
				if (length < dist){
					dist = length;
					closest_id = tab[0].neighbours[n];
				}
		
			}
		
		}
		
		// if has fluid around
		if (closest_id != -1){
			// get density
			dDensity[gid] = dDensity[ closest_id ];
		
			// recompute velocity
			vector v_end = solid_particle_v(closest_id, dPobj, dVel, gid);
			dVel[gid] = v_end;
		}
	
	}


	////// if not solid
	////if (gid < 512*8){
	////
	////	vector G = (ds + 0.00000001f) * 1000000000.f * (vector)(0., -9.8, 0., 0.);
	////	vector f = G + f_pressure * 0.2f;
	////	vector v_end = v + f/mass*dTime + viscosity * 0.5f;
	////	point p_end = p + v_end*dTime + f/mass*(point)(0.5*dTime*dTime);
	////	
	////
	////	p_end.w = 1.;
	////	v_end.w = 0.;
	////
	////	if (IsInsideSphere(p_end, Sphere1)){
	////		// update 1 dTime
	////		v_end = BounceSphere(p, v, Sphere1);
	////		p_end = p + v_end*dTime + f/mass*(point)(0.5*dTime*dTime);
	////	}
	////	
	////	if (p_end.y < -500){
	////		v_end = (vector)(0., 0., 0., 0.);
	////		dCobj[gid] = (color)(0., 0., 0., 1.);
	////	}
	////
	////	dPobj[gid] = p_end;
	////	dVel[gid] = v_end;
	////	dDensity[gid] = ds;
	////
	////}
	////// if is solid
	////else{
	////
	////	vector f = viscosity * 4.f;
	////
	////	vector v_end = v + f/mass*dTime;
	////
	////	v_end.w = 0.;
	////
	////	dPobj[gid] = p;
	////	dVel[gid] = v_end;
	////	dDensity[gid] = ds;
	////
	////}
}