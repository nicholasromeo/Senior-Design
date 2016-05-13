#ifndef REFERENCE_TEMPLATES_H
#define REFERENCE_TEMPLATES_H

//these point sets have barycentre {0, 0, 0} and are scaled such that their mean length is 1

const double points_sc[7][3] = {	
					{  0.            ,  0.            ,  0.            },
					{  0.            ,  0.            , -1.166666666667},
					{  0.            ,  0.            ,  1.166666666667},
					{  0.            , -1.166666666667,  0.            },
					{  0.            ,  1.166666666667,  0.            },
					{ -1.166666666667,  0.            ,  0.            },
					{  1.166666666667,  0.            ,  0.            },
};

const double points_fcc[13][3] = {	
					{  0.            ,  0.            ,  0.            },
					{  0.            ,  0.766032346285,  0.766032346285},
					{  0.            , -0.766032346285, -0.766032346285},
					{  0.            ,  0.766032346285, -0.766032346285},
					{  0.            , -0.766032346285,  0.766032346285},
					{  0.766032346285,  0.            ,  0.766032346285},
					{ -0.766032346285,  0.            , -0.766032346285},
					{  0.766032346285,  0.            , -0.766032346285},
					{ -0.766032346285,  0.            ,  0.766032346285},
					{  0.766032346285,  0.766032346285,  0.            },
					{ -0.766032346285, -0.766032346285,  0.            },
					{  0.766032346285, -0.766032346285,  0.            },
					{ -0.766032346285,  0.766032346285,  0.            },
	};

const double points_hcp[13][3] = {	
					{  0.            ,  0.            ,  0.            },
					{  0.766032346285,  0.            ,  0.766032346285},
					{ -0.255344115428, -1.021376461714, -0.255344115428},
					{  0.766032346285,  0.766032346285,  0.            },
					{ -0.255344115428, -0.255344115428, -1.021376461714},
					{  0.            ,  0.766032346285,  0.766032346285},
					{ -1.021376461714, -0.255344115428, -0.255344115428},
					{ -0.766032346285,  0.766032346285,  0.            },
					{  0.            ,  0.766032346285, -0.766032346285},
					{  0.766032346285,  0.            , -0.766032346285},
					{  0.766032346285, -0.766032346285,  0.            },
					{ -0.766032346285,  0.            ,  0.766032346285},
					{  0.            , -0.766032346285,  0.766032346285},
	};

const double points_ico[13][3] = {
					{  0.            ,  0.            ,  0.            },
					{  0.            ,  0.569542038129,  0.921538375715},
					{  0.            , -0.569542038129, -0.921538375715},
					{  0.            ,  0.569542038129, -0.921538375715},
					{  0.            , -0.569542038129,  0.921538375715},
					{ -0.569542038129, -0.921538375715,  0.            },
					{  0.569542038129,  0.921538375715,  0.            },
					{  0.569542038129, -0.921538375715,  0.            },
					{ -0.569542038129,  0.921538375715,  0.            },
					{ -0.921538375715,  0.            , -0.569542038129},
					{  0.921538375715,  0.            ,  0.569542038129},
					{  0.921538375715,  0.            , -0.569542038129},
					{ -0.921538375715,  0.            ,  0.569542038129},
	};

const double points_bcc[15][3] = {	
					{  0.            ,  0.            ,  0.            },
					{ -0.580127018922, -0.580127018922, -0.580127018922},
					{  0.580127018922,  0.580127018922,  0.580127018922},
					{  0.580127018922, -0.580127018922, -0.580127018922},
					{ -0.580127018922,  0.580127018922,  0.580127018922},
					{ -0.580127018922,  0.580127018922, -0.580127018922},
					{  0.580127018922, -0.580127018922,  0.580127018922},
					{ -0.580127018922, -0.580127018922,  0.580127018922},
					{  0.580127018922,  0.580127018922, -0.580127018922},
					{  0.            ,  0.            , -1.160254037844},
					{  0.            ,  0.            ,  1.160254037844},
					{  0.            , -1.160254037844,  0.            },
					{  0.            ,  1.160254037844,  0.            },
					{ -1.160254037844,  0.            ,  0.            },
					{  1.160254037844,  0.            ,  0.            },
	};
#endif

